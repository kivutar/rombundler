static const char *vshader_default_src =
	"attribute vec2 i_pos;\n"
	"attribute vec2 i_coord;\n"
	"varying vec2 o_coord;\n"
	"uniform mat4 u_mvp;\n"
	"void main() {\n"
		"o_coord = i_coord;\n"
		"gl_Position = vec4(i_pos, 0.0, 1.0) * u_mvp;\n"
	"}";

static const char *fshader_default_src =
	"varying vec2 o_coord;\n"
	"uniform sampler2D u_tex;\n"
	"void main() {\n"
		"gl_FragColor = texture2D(u_tex, o_coord);\n"
	"}";

static const char *fshader_zfastcrt_src =
	"precision mediump float;\n"
	"uniform vec2 u_tex_size;\n"
	"uniform sampler2D u_tex;\n"
	"varying vec2 o_coord;\n"
	"#define BLURSCALEX 0.45\n"
	"#define LOWLUMSCAN 5.0\n"
	"#define HILUMSCAN 10.0\n"
	"#define BRIGHTBOOST 1.25\n"
	"#define MASK_DARK 0.25\n"
	"#define MASK_FADE 0.8\n"
	"void main() {\n"
		"float maskFade = 0.3333*MASK_FADE;\n"
		"vec2 invDims = 1.0/u_tex_size.xy;\n"
		"vec2 p = o_coord * u_tex_size;\n"
		"vec2 i = floor(p) + 0.5;\n"
		"vec2 f = p - i;\n"
		"p = (i + 4.0*f*f*f)*invDims;\n"
		"p.x = mix(p.x , o_coord.x, BLURSCALEX);\n"
		"float Y = f.y*f.y;\n"
		"float YY = Y*Y;\n"
		"float whichmask = fract(o_coord.x*-0.4999);\n"
		"float mask = 1.0 + float(whichmask < 0.5) * -MASK_DARK;\n"
		"vec3 colour = texture2D(u_tex, p).rgb;\n"
		"float scanLineWeight = (BRIGHTBOOST - LOWLUMSCAN*(Y - 2.05*YY));\n"
		"float scanLineWeightB = 1.0 - HILUMSCAN*(YY-2.8*YY*Y);\n"
		"gl_FragColor = vec4(colour.rgb*mix(scanLineWeight*mask, scanLineWeightB, dot(colour.rgb,vec3(maskFade))), 1.0);\n"
	"}";

static const char *fshader_zfastlcd_src =
	"uniform vec2 u_tex_size;\n"
	"uniform sampler2D u_tex;\n"
	"varying vec2 o_coord;\n"
	"#define BORDERMULT 14.0\n"
	"#define GBAGAMMA 1.0\n"
	"void main() {\n"
		"vec2 texcoordInPixels = o_coord.xy * u_tex_size.xy;\n"
		"vec2 centerCoord = floor(texcoordInPixels.xy)+vec2(0.5,0.5);\n"
		"vec2 distFromCenter = abs(centerCoord - texcoordInPixels);\n"
		"vec2 invSize = 1.0/u_tex_size.xy;\n"
		"float Y = max(distFromCenter.x,(distFromCenter.y));\n"
		"Y=Y*Y;\n"
		"float YY = Y*Y;\n"
		"float YYY = YY*Y;\n"
		"float LineWeight = YY - 2.7*YYY;\n"
		"LineWeight = 1.0 - BORDERMULT*LineWeight;\n"
		"vec3 colour = texture2D(u_tex, invSize*centerCoord).rgb*LineWeight;\n"
		"if (GBAGAMMA > 0.5)\n"
			"colour.rgb*=0.6+0.4*(colour.rgb);\n"
		"gl_FragColor = vec4(colour.rgb , 1.0);\n"
	"}";
