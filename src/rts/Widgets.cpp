#include "rts/Widgets.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/FontManager.h"
#include "rts/ResourceManager.h"

namespace rts {

UIWidget *createWidget(const std::string &paramName) {
  auto type = strParam(paramName + ".type");
  if (type == "TextWidget") {
    return new TextWidget(paramName);
  } else {
    invariant(false, "No widget found for type " + type);
  }
}



UIWidget::UIWidget()
  : clickable_(false),
    pos_(-1.f), size_(0.f) {
}

void UIWidget::setClickable(const glm::vec2 &pos, const glm::vec2 &size) {
  pos_ = pos;
  size_ = size;
  clickable_ = true;
}

bool UIWidget::isClick(const glm::vec2 &pos) const {
  return clickable_ && pointInBox(pos, pos_, size_, 0.f);
}

bool UIWidget::handleClick(const glm::vec2 &pos) {
  if (!onClickListener_) {
    return false;
  }
  LOG(DEBUG) << "handleClick\n";
  return onClickListener_(pos);
}

void UIWidget::setOnClickListener(UIWidget::OnClickListener l) {
  onClickListener_ = l;
}



TextureWidget::TextureWidget(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    const std::string &texName)
  : SizedWidget(pos + size/2.f, size),
    texName_(texName) {
}

void TextureWidget::render(float dt) {
  auto tex = ResourceManager::get()->getTexture(texName_);
  drawTextureCenter(getCenter(), getSize(), tex);
}



TextWidget::TextWidget(const std::string &name, TextFunc func)
  : TextWidget(
      vec2Param(name + ".pos"),
      vec2Param(name + ".dim"),
      fltParam(name + ".fontHeight"),
      vec4Param(name + ".bgcolor"),
      func) {
  if (hasParam(name + ".text")) {
    setText(strParam(name + ".text"));
  }
}

TextWidget::TextWidget(
    const glm::vec2 &pos,
    const glm::vec2 &size,
    float fontHeight,
    const glm::vec4 &bgcolor,
    TextFunc textGetter)
  : SizedWidget(pos + size/2.f, size),
    height_(fontHeight),
    bgcolor_(bgcolor),
    textFunc_(textGetter) {
}

void TextWidget::setTextFunc(std::function<std::string()> func) {
  textFunc_ = func;
}

void TextWidget::setText(const std::string &text) {
  text_ = text;
}

void TextWidget::render(float dt) {
  std::string text = text_;
  if (textFunc_) {
    text = textFunc_();
  }
  drawRectCenter(getCenter(), getSize(), bgcolor_);
  FontManager::get()->drawString(text, getCenter() - getSize()/2.f, height_);
}

};  // rts
