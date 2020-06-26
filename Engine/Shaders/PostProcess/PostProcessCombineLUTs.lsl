(effect
    (refer Common.lsl)
    (refer PostProcess/TonemapCommon.lsl)
    (float4 ColorSaturation)
    (float4 ColorContrast)
    (float4 ColorGamma)
    (float4 ColorGain)
    (float4 ColorOffset)
    (float4 ColorSaturationShadows)
    (float4 ColorContrastShadows)
    (float4 ColorGammaShadows)
    (float4 ColorGainShadows)
    (float4 ColorOffsetShadows)
    (float4 ColorSaturationMidtones)
    (float4 ColorContrastMidtones)
    (float4 ColorGammaMidtones)
    (float4 ColorGainMidtones)
    (float4 ColorOffsetMidtones)
    (float4 ColorSaturationHighlights)
    (float4 ColorContrastHighlights)
    (float4 ColorGammaHighlights)
    (float4 ColorGainHighlights)
    (float4 ColorOffsetHighlights)
    (float ColorCorrectionShadowsMax)
    (float ColorCorrectionHighlightsMin)
    (float WhiteTemp)
    (float WhiteTint)
    (float BlueCorrection)
    (float ExpandGamut)
    (shader
"
static const float LUTSize = 32;

static const uint OutputGamut = 0;
        
// Accurate for 1000K < Temp < 15000K
// [Krystek 1985, \"An algorithm to calculate correlated colour temperature\"]
float2 PlanckianLocusChromaticity( float Temp )
{
	float u = ( 0.860117757f + 1.54118254e-4f * Temp + 1.28641212e-7f * Temp*Temp ) / ( 1.0f + 8.42420235e-4f * Temp + 7.08145163e-7f * Temp*Temp );
	float v = ( 0.317398726f + 4.22806245e-5f * Temp + 4.20481691e-8f * Temp*Temp ) / ( 1.0f - 2.89741816e-5f * Temp + 1.61456053e-7f * Temp*Temp );

	float x = 3*u / ( 2*u - 8*v + 4 );
	float y = 2*v / ( 2*u - 8*v + 4 );

	return float2(x,y);
}

// Accurate for 4000K < Temp < 25000K
// in: correlated color temperature
// out: CIE 1931 chromaticity
float2 D_IlluminantChromaticity( float Temp )
{
	// Correct for revision of Plank's law
	// This makes 6500 == D65
	Temp *= 1.4388 / 1.438;

	float x =	Temp <= 7000 ?
				0.244063 + ( 0.09911e3 + ( 2.9678e6 - 4.6070e9 / Temp ) / Temp ) / Temp :
				0.237040 + ( 0.24748e3 + ( 1.9018e6 - 2.0064e9 / Temp ) / Temp ) / Temp;
	
	float y = -3 * x*x + 2.87 * x - 0.275;

	return float2(x,y);
}

// Find closest color temperature to chromaticity
// [McCamy 1992, \"Correlated color temperature as an explicit function of chromaticity coordinates\"]
float CorrelatedColorTemperature( float x, float y )
{
	float n = (x - 0.3320) / (0.1858 - y);
	return -449 * n*n*n + 3525 * n*n - 6823.3 * n + 5520.33;
}

float2 PlanckianIsothermal( float Temp, float Tint )
{
	float u = ( 0.860117757f + 1.54118254e-4f * Temp + 1.28641212e-7f * Temp*Temp ) / ( 1.0f + 8.42420235e-4f * Temp + 7.08145163e-7f * Temp*Temp );
	float v = ( 0.317398726f + 4.22806245e-5f * Temp + 4.20481691e-8f * Temp*Temp ) / ( 1.0f - 2.89741816e-5f * Temp + 1.61456053e-7f * Temp*Temp );

	float ud = ( -1.13758118e9f - 1.91615621e6f * Temp - 1.53177f * Temp*Temp ) / Square( 1.41213984e6f + 1189.62f * Temp + Temp*Temp );
	float vd = (  1.97471536e9f - 705674.0f * Temp - 308.607f * Temp*Temp ) / Square( 6.19363586e6f - 179.456f * Temp + Temp*Temp );

	float2 uvd = normalize( float2( u, v ) );

	// Correlated color temperature is meaningful within +/- 0.05
	u += -uvd.y * Tint * 0.05;
	v +=  uvd.x * Tint * 0.05;
	
	float x = 3*u / ( 2*u - 8*v + 4 );
	float y = 2*v / ( 2*u - 8*v + 4 );

	return float2(x,y);
}

float3 WhiteBalance( float3 LinearColor )
{
	float2 SrcWhiteDaylight = D_IlluminantChromaticity( WhiteTemp );
	float2 SrcWhitePlankian = PlanckianLocusChromaticity( WhiteTemp );

	float2 SrcWhite = WhiteTemp < 4000 ? SrcWhitePlankian : SrcWhiteDaylight;
	float2 D65White = float2( 0.31270,  0.32900 );

	{
		// Offset along isotherm
		float2 Isothermal = PlanckianIsothermal( WhiteTemp, WhiteTint ) - SrcWhitePlankian;
		SrcWhite += Isothermal;
	}

	float3x3 WhiteBalanceMat = ChromaticAdaptation( SrcWhite, D65White );
	WhiteBalanceMat = mul( XYZ_2_sRGB_MAT, mul( WhiteBalanceMat, sRGB_2_XYZ_MAT ) );

	return mul( WhiteBalanceMat, LinearColor );
}

float3 ColorCorrect( float3 WorkingColor,
	float4 ColorSaturation,
	float4 ColorContrast,
	float4 ColorGamma,
	float4 ColorGain,
	float4 ColorOffset )
{
	// TODO optimize
	float Luma = dot( WorkingColor, AP1_RGB2Y );
	WorkingColor = max( 0, lerp( Luma.xxx, WorkingColor, ColorSaturation.xyz*ColorSaturation.w ) );
	WorkingColor = pow( WorkingColor * (1.0 / 0.18), ColorContrast.xyz*ColorContrast.w ) * 0.18;
	WorkingColor = pow( WorkingColor, 1.0 / (ColorGamma.xyz*ColorGamma.w) );
	WorkingColor = WorkingColor * (ColorGain.xyz * ColorGain.w) + (ColorOffset.xyz + ColorOffset.w);
	return WorkingColor;
}

// Nuke-style Color Correct
float3 ColorCorrectAll( float3 WorkingColor )
{
	float Luma = dot( WorkingColor, AP1_RGB2Y );

	// Shadow CC
	float3 CCColorShadows = ColorCorrect(WorkingColor, 
		ColorSaturationShadows*ColorSaturation, 
		ColorContrastShadows*ColorContrast, 
		ColorGammaShadows*ColorGamma, 
		ColorGainShadows*ColorGain, 
		ColorOffsetShadows+ColorOffset);
	float CCWeightShadows = 1- smoothstep(0, ColorCorrectionShadowsMax, Luma);
	
	// Highlight CC
	float3 CCColorHighlights = ColorCorrect(WorkingColor, 
		ColorSaturationHighlights*ColorSaturation, 
		ColorContrastHighlights*ColorContrast, 
		ColorGammaHighlights*ColorGamma, 
		ColorGainHighlights*ColorGain, 
		ColorOffsetHighlights+ColorOffset);
	float CCWeightHighlights = smoothstep(ColorCorrectionHighlightsMin, 1, Luma);

	// Midtone CC
	float3 CCColorMidtones = ColorCorrect(WorkingColor, 
		ColorSaturationMidtones*ColorSaturation, 
		ColorContrastMidtones*ColorContrast, 
		ColorGammaMidtones*ColorGamma, 
		ColorGainMidtones*ColorGain, 
		ColorOffsetMidtones+ColorOffset);
	float CCWeightMidtones = 1 - CCWeightShadows - CCWeightHighlights;

	// Blend Shadow, Midtone and Highlight CCs
	float3 WorkingColorSMH = CCColorShadows*CCWeightShadows + CCColorMidtones*CCWeightMidtones + CCColorHighlights*CCWeightHighlights;
	
	return WorkingColorSMH;
}

uint GetOutputDevice()
{
	return 0;
}

float4 CombineLUTsCommon(float2 InUV, uint InLayerIndex)
{
    float4 Neutral;
    {
        float2 UV = InUV - float2(0.5f / LUTSize, 0.5f / LUTSize);

        Neutral = float4(UV * LUTSize / (LUTSize - 1), InLayerIndex / (LUTSize - 1), 0);
    }

    float4 OutColor = 0;
	
	// AP1 to Working space matrices
	const float3x3 sRGB_2_AP1 = mul( XYZ_2_AP1_MAT, mul( D65_2_D60_CAT, sRGB_2_XYZ_MAT ) );
	const float3x3 AP1_2_sRGB = mul( XYZ_2_sRGB_MAT, mul( D60_2_D65_CAT, AP1_2_XYZ_MAT ) );

	const float3x3 AP0_2_AP1 = mul( XYZ_2_AP1_MAT, AP0_2_XYZ_MAT );
	const float3x3 AP1_2_AP0 = mul( XYZ_2_AP0_MAT, AP1_2_XYZ_MAT );

	const float3x3 AP1_2_Output  = OuputGamutMappingMatrix( OutputGamut );

	float3 LUTEncodedColor = Neutral.rgb;
	float3 LinearColor;

    LinearColor = LogToLin( LUTEncodedColor ) - LogToLin( 0 );

    float3 BalancedColor = WhiteBalance( LinearColor );
	float3 ColorAP1 = mul( sRGB_2_AP1, BalancedColor );

    // Expand bright saturated colors outside the sRGB gamut to fake wide gamut rendering.
	{
        float  LumaAP1 = dot( ColorAP1, AP1_RGB2Y );
		float3 ChromaAP1 = ColorAP1 / LumaAP1;

		float ChromaDistSqr = dot( ChromaAP1 - 1, ChromaAP1 - 1 );
		float ExpandAmount = ( 1 - exp2( -4 * ChromaDistSqr ) ) * ( 1 - exp2( -4 * ExpandGamut * LumaAP1*LumaAP1 ) );

		// Bizarre matrix but this expands sRGB to between P3 and AP1
		// CIE 1931 chromaticities:	x		y
		//				Red:		0.6965	0.3065
		//				Green:		0.245	0.718
		//				Blue:		0.1302	0.0456
		//				White:		0.3127	0.329
		const float3x3 Wide_2_XYZ_MAT = 
		{
			0.5441691,  0.2395926,  0.1666943,
			0.2394656,  0.7021530,  0.0583814,
			-0.0023439,  0.0361834,  1.0552183,
		};

		const float3x3 Wide_2_AP1 = mul( XYZ_2_AP1_MAT, Wide_2_XYZ_MAT );
		const float3x3 ExpandMat = mul( Wide_2_AP1, AP1_2_sRGB );

		float3 ColorExpand = mul( ExpandMat, ColorAP1 );
		ColorAP1 = lerp( ColorAP1, ColorExpand, ExpandAmount );
    }

	ColorAP1 = ColorCorrectAll( ColorAP1 );

	// Store for Legacy tonemap later and for Linear HDR output without tone curve
	float3 GradedColor = mul( AP1_2_sRGB, ColorAP1 );

	const float3x3 BlueCorrect =
	{
		0.9404372683, -0.0183068787, 0.0778696104,
		0.0083786969,  0.8286599939, 0.1629613092,
		0.0005471261, -0.0008833746, 1.0003362486
	};
	const float3x3 BlueCorrectInv =
	{
		1.06318,     0.0233956, -0.0865726,
		-0.0106337,   1.20632,   -0.19569,
		-0.000590887, 0.00105248, 0.999538
	};
	const float3x3 BlueCorrectAP1    = mul( AP0_2_AP1, mul( BlueCorrect,    AP1_2_AP0 ) );
	const float3x3 BlueCorrectInvAP1 = mul( AP0_2_AP1, mul( BlueCorrectInv, AP1_2_AP0 ) );

	// Blue correction
	ColorAP1 = lerp( ColorAP1, mul( BlueCorrectAP1, ColorAP1 ), BlueCorrection );

	// Tonemapped color in the AP1 gamut
	ColorAP1 = FilmToneMap( ColorAP1 );

	// Uncorrect blue to maintain white point
	ColorAP1 = lerp( ColorAP1, mul( BlueCorrectInvAP1, ColorAP1 ), BlueCorrection );

	// Convert from AP1 to sRGB and clip out-of-gamut values
	float3 FilmColor = max(0, mul( AP1_2_sRGB, ColorAP1 ));

	//TODO apply math color correction on top to texture based solution
	//FilmColor = ColorCorrection( FilmColor );

	// Apply \"gamma\" curve adjustment.
	FilmColor = pow( max(0, FilmColor), InverseGamma.y );

	half3 OutDeviceColor = 0;

	// sRGB, user specified gamut
	if( GetOutputDevice() == 0 )
	{		
		// Convert from sRGB to specified output gamut	
		//float3 OutputGamutColor = mul( AP1_2_Output, mul( sRGB_2_AP1, FilmColor ) );

		// Default parameters seem to cancel out (sRGB->XYZ->AP1->XYZ->sRGB), so should be okay for a temp fix
		float3 OutputGamutColor = FilmColor;

		// Apply conversion to sRGB (this must be an exact sRGB conversion else darks are bad).
		OutDeviceColor = LinearToSrgb( OutputGamutColor );
	}

	// Better to saturate(lerp(a,b,t)) than lerp(saturate(a),saturate(b),t)
	OutColor.rgb = OutDeviceColor / 1.05;
	OutColor.a = 0;

	return OutColor;
}

void MainPS(WriteToSliceGeometryOutput Input, out float4 OutColor : SV_Target0)
{
	OutColor = CombineLUTsCommon(Input.Vertex.UV, Input.LayerIndex);
}
"
    )
)