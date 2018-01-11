// sample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../include/glad/glad.h"
#include "../include/GLFW/glfw3.h"
#pragma comment(lib, "../lib/glfw3.lib")

#include <stdio.h>
#include <stdlib.h>

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

GLchar * vs[] =
{
  "attribute vec4 a_vert; \n"
  "attribute vec2 a_text; \n"
  "varying vec2    v_text; \n"
  "void main(){ \n"
  "  gl_Position = a_vert; \n"
  "  v_text            = a_text; \n"
  "} \n"
};

GLchar * fs[] =
{
  "#ifdef GL_ES \n"
  "precision highp float; \n"
  "#endif \n"
  "varying vec2 v_text; \n"
  "uniform sampler2D y_text; \n"
  "uniform sampler2D uv_text; \n"
  "void main (void){ \n"
  "  float r, g, b, y, u, v; \n"
  "  y = texture2D(y_text,   v_text).r; \n"
  "  v = texture2D(uv_text, v_text).a - 0.5; \n"
  "  u = texture2D(uv_text, v_text).r - 0.5; \n"
  "  r = y + 1.13983*v; \n"
  "  g = y - 0.39465*u - 0.58060*v; \n"
  "  b = y + 2.03211*u; \n"
  "  gl_FragColor = vec4(r, g, b, 1.0); \n"
  "} \n"
};


class stream
{
public:
  virtual ~stream() {}

  stream(const int w, const int h) :
    pixel_w(w),
    pixel_h(h),
    pitch(4096),
    raw_frame(pixel_w*pixel_h * 3 / 2),
    half_pitch(pitch/2),
    infile(nullptr)
  {
    init_shader();
  };

  void render()
  {
    if (fread(buf, 1, pixel_w * pixel_h*3/2, infile) != pixel_w * pixel_h*3/2)
    {
      fseek(infile, 0, SEEK_SET);
      return;
    }

    // nv12 data
    copy0(plane0, buf, pixel_w);
    copy1(plane1, buf, pixel_w);

    //Clear
    glClearColor(0.0, 255, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    //Y
    glBindTexture(GL_TEXTURE_2D, id_y);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, pixel_w, pixel_h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, plane0);

    //UV
    glBindTexture(GL_TEXTURE_2D, id_uv);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, pixel_w / 2, pixel_h / 2, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, plane1);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  }

  bool open(const char* str)
  {
    if (fopen_s(&infile, str, "rb") != NULL)
    {
      printf("cannot open %s\n", str);
      return false;
    }
    return true;
  }

private:
  const int pixel_w;
  const int pixel_h;
  int pitch;
  int raw_frame;
  int half_pitch;
  FILE *infile;
  unsigned char buf[4096 * 2048];

  //nv12
  unsigned char plane0[4096 * 2048];
  unsigned char plane1[4096 * 2048];

  // Y
  void copy0(UINT8 * pDest, UINT8* pData, int pixel_w)
  {
    for (int i = 0, k = 0; i < raw_frame; k++, i += pitch)
      memcpy(pDest + k * pixel_w, pData + i, pixel_w);
  }

  // Cb, Cr
  void copy1(UINT8 * pDest, UINT8* pData, int pixel_w)
  {
    for (int i = 0, k = 0; i < raw_frame; k++, i += pitch)
      if ((k & 0x1) == 0)
        memcpy(pDest + k / 2 * pixel_w, pData + i + half_pitch, pixel_w);
  }

  GLuint prog;
  GLuint id_y, id_uv; // Texture id
  GLuint textureUniformY, textureUniformUV;

  void init_shader()
  {
    GLint vertCompiled, fragCompiled, linked;

    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(v, 1, vs, NULL);
    glShaderSource(f, 1, fs, NULL);
    glCompileShader(v);
    glGetShaderiv(v, GL_COMPILE_STATUS, &vertCompiled);
    glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &fragCompiled);

    prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glBindAttribLocation(prog, 4,  "a_vert");            //vertex   4
    glBindAttribLocation(prog, 2,  "a_text");            //texture 2
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    glUseProgram(prog);

    glDeleteShader(v);
    glDeleteShader(f);

    textureUniformY = glGetUniformLocation(prog, "y_text");
    textureUniformUV = glGetUniformLocation(prog, "uv_text");

    static const GLfloat vertexVertices[] =
    {
      -1.0f, -1.0f,
      1.0f, -1.0f,
      -1.0f,  1.0f,
      1.0f,  1.0f,
    };

    static const GLfloat textureVertices[] =
    {
      0.0f,  1.0f,
      1.0f,  1.0f,
      0.0f,  0.0f,
      1.0f,  0.0f,
    };

    glVertexAttribPointer(4, 2, GL_FLOAT, 0, 0, vertexVertices);   //vertex  4
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(2, 2, GL_FLOAT, 0, 0, textureVertices);  //texture 2
    glEnableVertexAttribArray(2);

    //init texture
    glGenTextures(1, &id_y);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id_y);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glUniform1i(textureUniformY, 0);

    glGenTextures(1, &id_uv);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, id_uv);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glUniform1i(textureUniformUV, 1);

  }

};

int main()
{
  // 960x540 NV12 YUV data
  const int pixel_w =  960;
  const int pixel_h =  540;

  // GLFW
  GLFWwindow* window;
  glfwSetErrorCallback(error_callback);

  if (!glfwInit())  exit(EXIT_FAILURE);

  window = glfwCreateWindow(pixel_w, pixel_h, "Sample", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(2);

  stream *my_stream  = new stream(pixel_w, pixel_h);
  if (my_stream->open("../../nv12.yuv"))
  {
    while (!glfwWindowShouldClose(window))
    {
      my_stream->render();
      glfwSwapBuffers(window);
      glfwPollEvents();
    }

    delete my_stream;
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
  }

  delete my_stream;
  exit(EXIT_SUCCESS);
  return -1;
}

