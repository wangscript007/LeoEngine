(effect
	(cbuffer obj
		(float4x4 world)
	)
	(cbuffer camera
		(float3 camera_pos)
		(float4x4 viewproj)
	)
	(shader
		"
		void ForwardVS(in float3 Postion:POSITION,
						in float4 Tangent_Quat:TANGENT,
						in float2 TexCoord:TEXCOORD,
					out float4 ClipPos:SV_POSITION
		)
		{
			float3 WorldPos = mul(float4(Postion,1.0f),world).xyz;

			ClipPos = mul(float4(WorldPos,1.0f),viewproj);
		}

		void ForwardPS(in float4 ClipPos:SV_POSITION
		)
		{
		}
		"
	)
	(technique (name PreZ)
		(pass (name p0)
			(vertex_shader ForwardVS)
			(pixel_shader ForwardPS)
			(color_write_mask_0 0)
		)
	)
)