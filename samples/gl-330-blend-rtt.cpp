//**********************************
// OpenGL Render to texture blending 
// 28/05/2010 - 16/06/2010
//**********************************
// Christophe Riccio
// ogl-samples@g-truc.net
//**********************************
// G-Truc Creation
// www.g-truc.net
//**********************************

#include <glf/glf.hpp>

namespace
{
	std::string const SAMPLE_NAME("OpenGL Render to texture blending");
	std::string const VERTEX_SHADER_SOURCE1(glf::DATA_DIRECTORY + "gl-330/mrt.vert");
	std::string const FRAGMENT_SHADER_SOURCE1(glf::DATA_DIRECTORY + "gl-330/mrt.frag");
	std::string const VERTEX_SHADER_SOURCE2(glf::DATA_DIRECTORY + "gl-330/image-2d.vert");
	std::string const FRAGMENT_SHADER_SOURCE2(glf::DATA_DIRECTORY + "gl-330/image-2d.frag");
	std::string const TEXTURE_DIFFUSE(glf::DATA_DIRECTORY + "kueken320-rgb8.tga");
	glm::ivec2 const FRAMEBUFFER_SIZE(640, 480);
	int const SAMPLE_SIZE_WIDTH(640);
	int const SAMPLE_SIZE_HEIGHT(480);
	int const SAMPLE_MAJOR_VERSION(3);
	int const SAMPLE_MINOR_VERSION(3);

	glf::window Window(glm::ivec2(SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT));

	struct vertex
	{
		vertex
		(
			glm::vec2 const & Position,
			glm::vec2 const & Texcoord
		) :
			Position(Position),
			Texcoord(Texcoord)
		{}

		glm::vec2 Position;
		glm::vec2 Texcoord;
	};

	// With DDS textures, v texture coordinate are reversed, from top to bottom
	GLsizei const VertexCount = 6;
	GLsizeiptr const VertexSize = VertexCount * sizeof(vertex);
	vertex const VertexData[VertexCount] =
	{
		vertex(glm::vec2(-1.0f,-1.0f), glm::vec2(0.0f, 0.0f)),
		vertex(glm::vec2( 1.0f,-1.0f), glm::vec2(1.0f, 0.0f)),
		vertex(glm::vec2( 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
		vertex(glm::vec2( 1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
		vertex(glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
		vertex(glm::vec2(-1.0f,-1.0f), glm::vec2(0.0f, 0.0f))
	};

	enum texture_type
	{
		TEXTURE_RGB8,
		TEXTURE_R,
		TEXTURE_G,
		TEXTURE_B,
		TEXTURE_MAX
	};

	GLuint FramebufferName = 0;
	GLuint VertexArrayName = 0;

	GLuint ProgramNameSingle = 0;
	GLint UniformMVPSingle = 0;
	GLint UniformDiffuseSingle = 0;

	GLuint ProgramNameMultiple = 0;
	GLint UniformMVPMultiple = 0;
	GLint UniformDiffuseMultiple = 0;

	GLuint BufferName = 0;
	GLuint Texture2DName[TEXTURE_MAX];
	GLuint SamplerName = 0;

	glm::ivec4 Viewport[TEXTURE_MAX];

}//namespace

bool initDebugOutput()
{
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	glDebugMessageCallbackARB(&glf::debugOutput, NULL);

	return glf::checkError("initDebugOutput");
}

GLuint initShader
(
	std::string const & Filename, 
	GLenum Stage
)
{
	bool Validated = true;

	GLuint ShaderName = 0;
	if(Validated)
	{
		std::string Source0 = glf::loadFile(Filename);
		char const * Source = Source0.c_str();
		ShaderName = glCreateShader(Stage);
		glShaderSource(ShaderName, 1, &Source, NULL);
		glCompileShader(ShaderName);
		Validated = glf::checkShader(ShaderName, Source);
	}
	
	glf::checkError("initShader");

	return ShaderName;
}

bool initProgram()
{
	bool Validated = true;

	// Create program
	if(Validated)
	{
		GLuint VertexShaderName = glf::createShader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE1);
		GLuint FragmentShaderName = glf::createShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE1);

		ProgramNameMultiple = glCreateProgram();
		glAttachShader(ProgramNameMultiple, VertexShaderName);
		glAttachShader(ProgramNameMultiple, FragmentShaderName);
		glDeleteShader(VertexShaderName);
		glDeleteShader(FragmentShaderName);

		glLinkProgram(ProgramNameMultiple);
		Validated = glf::checkProgram(ProgramNameMultiple);
	}

	if(Validated)
	{
		UniformMVPMultiple = glGetUniformLocation(ProgramNameMultiple, "MVP");
		UniformDiffuseMultiple = glGetUniformLocation(ProgramNameMultiple, "Diffuse");
	}

	// Create program
	if(Validated)
	{
		GLuint VertexShaderName = glf::createShader(GL_VERTEX_SHADER, VERTEX_SHADER_SOURCE2);
		GLuint FragmentShaderName = glf::createShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SOURCE2);

		ProgramNameSingle = glCreateProgram();
		glAttachShader(ProgramNameSingle, VertexShaderName);
		glAttachShader(ProgramNameSingle, FragmentShaderName);
		glDeleteShader(VertexShaderName);
		glDeleteShader(FragmentShaderName);

		glLinkProgram(ProgramNameSingle);
		Validated = glf::checkProgram(ProgramNameSingle);
	}

	if(Validated)
	{
		UniformMVPSingle = glGetUniformLocation(ProgramNameSingle, "MVP");
		UniformDiffuseSingle = glGetUniformLocation(ProgramNameSingle, "Diffuse");
	}

	return glf::checkError("initProgram");
}

bool initArrayBuffer()
{
	glGenBuffers(1, &BufferName);

    glBindBuffer(GL_ARRAY_BUFFER, BufferName);
    glBufferData(GL_ARRAY_BUFFER, VertexSize, VertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return glf::checkError("initArrayBuffer");
}

bool initSampler()
{
	glGenSamplers(1, &SamplerName);
	glSamplerParameteri(SamplerName, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(SamplerName, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(SamplerName, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(SamplerName, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(SamplerName, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameterfv(SamplerName, GL_TEXTURE_BORDER_COLOR, &glm::vec4(0.0f)[0]);
	glSamplerParameterf(SamplerName, GL_TEXTURE_MIN_LOD, -1000.f);
	glSamplerParameterf(SamplerName, GL_TEXTURE_MAX_LOD, 1000.f);
	glSamplerParameterf(SamplerName, GL_TEXTURE_LOD_BIAS, 0.0f);
	glSamplerParameteri(SamplerName, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glSamplerParameteri(SamplerName, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	return glf::checkError("initSampler");
}

bool initTexture2D()
{
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(TEXTURE_MAX, Texture2DName);

	gli::texture2D Image = gli::load(TEXTURE_DIFFUSE);

	// Load image
	glBindTexture(GL_TEXTURE_2D, Texture2DName[TEXTURE_RGB8]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	for(std::size_t Level = 0; Level < Image.levels(); ++Level)
	{
		glTexImage2D(
			GL_TEXTURE_2D, 
			GLint(Level), 
			GL_RGB,
			GLsizei(Image[Level].dimensions().x), 
			GLsizei(Image[Level].dimensions().y), 
			0,  
			GL_RGB, 
			GL_UNSIGNED_BYTE, 
			Image[Level].data());
	}

	glBindTexture(GL_TEXTURE_2D, Texture2DName[TEXTURE_R]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ZERO);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ZERO);

	glBindTexture(GL_TEXTURE_2D, Texture2DName[TEXTURE_G]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ZERO);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ZERO);

	glBindTexture(GL_TEXTURE_2D, Texture2DName[TEXTURE_B]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ZERO);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ZERO);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ZERO);

	for(int i = TEXTURE_R; i <= TEXTURE_B; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, Texture2DName[i]);
		glTexImage2D(
			GL_TEXTURE_2D, 
			0, 
			GL_RED,
			GLsizei(Image[0].dimensions().x), 
			GLsizei(Image[0].dimensions().y), 
			0,  
			GL_RGB, 
			GL_UNSIGNED_BYTE, 
			0);
	}

	return glf::checkError("initTexture2D");
}

bool initFramebuffer()
{
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	for(std::size_t i = TEXTURE_R; i <= TEXTURE_B; ++i)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + GLenum(i - TEXTURE_R), Texture2DName[i], 0);

	GLenum DrawBuffers[3];
	DrawBuffers[0] = GL_COLOR_ATTACHMENT0;
	DrawBuffers[1] = GL_COLOR_ATTACHMENT1;
	DrawBuffers[2] = GL_COLOR_ATTACHMENT2;

	glDrawBuffers(3, DrawBuffers);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	return glf::checkError("initFramebuffer");
}

bool initVertexArray()
{
	glGenVertexArrays(1, &VertexArrayName);
    glBindVertexArray(VertexArrayName);
		glBindBuffer(GL_ARRAY_BUFFER, BufferName);
		glVertexAttribPointer(glf::semantic::attr::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), GLF_BUFFER_OFFSET(0));
		glVertexAttribPointer(glf::semantic::attr::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), GLF_BUFFER_OFFSET(sizeof(glm::vec2)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glEnableVertexAttribArray(glf::semantic::attr::POSITION);
		glEnableVertexAttribArray(glf::semantic::attr::TEXCOORD);
	glBindVertexArray(0);

	return glf::checkError("initVertexArray");
}

bool initBlend()
{
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_COLOR, GL_SRC_COLOR, GL_ZERO, GL_ZERO);

	glEnablei(GL_BLEND, 0);
	glEnablei(GL_BLEND, 1);
	glEnablei(GL_BLEND, 2);
	glEnablei(GL_BLEND, 3);

	glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glColorMaski(1, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	glColorMaski(2, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	glColorMaski(3, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);

	return glf::checkError("initBlend");
}

bool begin()
{
	Viewport[TEXTURE_RGB8] = glm::ivec4(0, 0, Window.Size >> 1);
	Viewport[TEXTURE_R] = glm::ivec4(Window.Size.x >> 1, 0, Window.Size >> 1);
	Viewport[TEXTURE_G] = glm::ivec4(Window.Size.x >> 1, Window.Size.y >> 1, Window.Size >> 1);
	Viewport[TEXTURE_B] = glm::ivec4(0, Window.Size.y >> 1, Window.Size >> 1);

	bool Validated = true;
	Validated = Validated && glf::checkGLVersion(SAMPLE_MAJOR_VERSION, SAMPLE_MINOR_VERSION);
	Validated = Validated && glf::checkExtension("GL_ARB_viewport_array");
	Validated = Validated && glf::checkExtension("GL_ARB_separate_shader_objects");

	if(Validated && glf::checkExtension("GL_ARB_debug_output"))
		Validated = initDebugOutput();
	if(Validated)
		Validated = initBlend();
	if(Validated)
		Validated = initProgram();
	if(Validated)
		Validated = initArrayBuffer();
	if(Validated)
		Validated = initVertexArray();
	if(Validated)
		Validated = initSampler();
	if(Validated)
		Validated = initTexture2D();
	if(Validated)
		Validated = initFramebuffer();

	return Validated && glf::checkError("begin");
}

bool end()
{
	glDeleteBuffers(1, &BufferName);
	glDeleteProgram(ProgramNameMultiple);
	glDeleteProgram(ProgramNameSingle);
	glDeleteTextures(TEXTURE_MAX, Texture2DName);
	glDeleteFramebuffers(1, &FramebufferName);
	glDeleteSamplers(1, &SamplerName);

	return glf::checkError("end");
}

void display()
{
	// Pass 1
	{
		// Compute the MVP (Model View Projection matrix)
		glm::mat4 Projection = glm::ortho(-1.0f, 1.0f,-1.0f, 1.0f, -1.0f, 1.0f);
		glm::mat4 ViewTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		glm::mat4 View = ViewTranslate;
		glm::mat4 Model = glm::mat4(1.0f);
		glm::mat4 MVP = Projection * View * Model;

		glProgramUniformMatrix4fv(ProgramNameMultiple, UniformMVPMultiple, 1, GL_FALSE, &MVP[0][0]);
		glProgramUniform1i(ProgramNameMultiple, UniformDiffuseMultiple, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		glViewportIndexedf(0, 0, 0, float(FRAMEBUFFER_SIZE.x >> 1), float(FRAMEBUFFER_SIZE.y >> 1));
		glClearBufferfv(GL_COLOR, 0, &glm::vec4(1.0f)[0]);

		glUseProgram(ProgramNameMultiple);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture2DName[TEXTURE_RGB8]);
		glBindSampler(0, SamplerName);
		glBindVertexArray(VertexArrayName);

		glDrawArraysInstanced(GL_TRIANGLES, 0, VertexCount, 1);
	}

	// Pass 2
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearBufferfv(GL_COLOR, 0, &glm::vec4(1.0f)[0]);

	glUseProgram(ProgramNameSingle);

	{
		glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, 1.0f,-1.0f, -1.0f, 1.0f);
		glm::mat4 View = glm::mat4(1.0f);
		glm::mat4 Model = glm::mat4(1.0f);
		glm::mat4 MVP = Projection * View * Model;

		glProgramUniformMatrix4fv(ProgramNameSingle, UniformMVPSingle, 1, GL_FALSE, &MVP[0][0]);
		glProgramUniform1i(ProgramNameSingle, UniformDiffuseSingle, 0);
	}

	for(std::size_t i = 0; i < TEXTURE_MAX; ++i)
	{
		glViewportIndexedfv(0, &glm::vec4(Viewport[i])[0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture2DName[i]);
		glBindSampler(0, SamplerName);
		glBindVertexArray(VertexArrayName);

		glDrawArraysInstanced(GL_TRIANGLES, 0, VertexCount, 1);
	}

	glf::checkError("display");
	glf::swapBuffers();
}

int main(int argc, char* argv[])
{
	return glf::run(
		argc, argv,
		glm::ivec2(::SAMPLE_SIZE_WIDTH, ::SAMPLE_SIZE_HEIGHT), 
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB, ::SAMPLE_MAJOR_VERSION, 
		::SAMPLE_MINOR_VERSION);
}
