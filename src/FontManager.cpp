#include "FontManager.h"
#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>

const float widthRatio = 0.75f;

FontManager::FontManager() :
    numberTex_(0),
    letterTex_(0)
{
    initialize();
}

FontManager::~FontManager()
{
    freeTexture(numberTex_);
    freeTexture(letterTex_);

    // TODO free/unattach program/shader
}

void FontManager::renderNumber(const glm::mat4 &transform,
        const glm::vec3 &color, float num)
{
    assert(num >= 0);
    std::string s = floatStr(num);
    size_t len = s.length();

    float offset = static_cast<float>(len) / -2.f + 0.5f;
    offset *= widthRatio;

    for (size_t i = 0; i < len; i++)
    {
        glm::mat4 final_transform =
            glm::translate(transform, glm::vec3(offset, 0.f, 0.f));

        renderDigit(final_transform, color, s[i]);

        offset += widthRatio;
    }
}

void FontManager::renderString(const glm::mat4 &transform,
        const glm::vec3 &color, const std::string &str)
{
    const size_t len = str.length();

    float offset = static_cast<float>(len) / -2.f + 0.5f;
    offset *= widthRatio;

    for (size_t i = 0; i < len; i++)
    {
        glm::mat4 final_transform =
            glm::translate(transform, glm::vec3(offset, 0.f, 0.f));

        char ch = str[i];

        if (isalpha(ch))
            renderCharacter(final_transform, color, ch);
        else if (isdigit(ch))
            renderDigit(final_transform, color, ch);
        else if (isspace(ch))
        {
            // nothing
        }
        else
            assert(false && "Can't render unknown character");
            

        offset += widthRatio;
    }
}

int FontManager::numDigits(int n)
{
    int count;
    for (count = 0; n != 0; n /= 10, count++);

    return std::max(count, 1);
}


// TODO(zack) get rid of this in favor of a full ascii map
void FontManager::renderDigit(const glm::mat4 &transform,
    const glm::vec3 &color, char dig)
{
    char digit = dig - '0';

    // FIXME(zack) remove this special case
    if (dig == '.')
    {
      return;
    }

    glm::vec2 size(1.f/10.f, 1.f);
    glm::vec2 pos(size.x * static_cast<float>(digit), 0.f);

    // Set up the shader
    GLuint posUniform = glGetUniformLocation(numProgram_, "pos");
    GLuint sizeUniform = glGetUniformLocation(numProgram_, "size");
    GLuint colorUniform = glGetUniformLocation(numProgram_, "color");

    glUseProgram(numProgram_);
    glUniform2fv(posUniform, 1, glm::value_ptr(pos));
    glUniform2fv(sizeUniform, 1, glm::value_ptr(size));
    glUniform3fv(colorUniform, 1, glm::value_ptr(color));
    glUseProgram(0);
    // Set up the texture
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, numberTex_);

    renderRectangleProgram(glm::scale(transform, glm::vec3(0.66f, -1.f, 1.f)));

    glDisable(GL_TEXTURE_2D);
}

void FontManager::renderCharacter(const glm::mat4 &transform,
    const glm::vec3 &color, char ch)
{
    ch = toupper(ch);
    int ind = ch - 'A';
    assert(ind >= 0 && ind < 26);

    glm::vec2 size(1.f/26.f, 1.f);
    glm::vec2 pos(size.x * static_cast<float>(ind), 0.f);

    // Set up the shader
    GLuint posUniform = glGetUniformLocation(numProgram_, "pos");
    GLuint sizeUniform = glGetUniformLocation(numProgram_, "size");
    GLuint colorUniform = glGetUniformLocation(numProgram_, "color");

    glUseProgram(numProgram_);
    glUniform2fv(posUniform, 1, glm::value_ptr(pos));
    glUniform2fv(sizeUniform, 1, glm::value_ptr(size));
    glUniform3fv(colorUniform, 1, glm::value_ptr(color));
    glUseProgram(0);
    // Set up the texture
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, letterTex_);

    renderRectangleProgram(glm::scale(transform, glm::vec3(0.66f, -1.f, 1.f)));

    glDisable(GL_TEXTURE_2D);
}

int FontManager::numChars(float f)
{
    return floatStr(f).length();
}

std::string FontManager::floatStr(float f)
{
    std::stringstream ss;
    if (f != (int) f)
    {
        ss.precision(2);
        ss.setf(std::ios::fixed, std::ios::floatfield);
    }
    ss << f;

    return ss.str();
}

FontManager * FontManager::get()
{
    static FontManager fm;
    return &fm;
}

void FontManager::initialize()
{
    numberTex_ = makeTexture("images/numbers-64.png");
    letterTex_ = makeTexture("images/capital-letters-64.png");

    numProgram_ = loadProgram("shaders/num.v.glsl", "shaders/num.f.glsl");
    assert(numProgram_);
}
