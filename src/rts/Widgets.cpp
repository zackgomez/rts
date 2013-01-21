#include "rts/Widgets.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/FontManager.h"
#include "rts/ResourceManager.h"
#include "rts/UI.h"

namespace rts {

void createWidgets(const std::string &widgetGroupName) {
  Json::Value group = ParamReader::get()->getParam(widgetGroupName);
  for (auto it = group.begin(); it != group.end(); it++) {
    auto name = widgetGroupName + '.' + it.memberName();
    auto widget = createWidget(name);
    UI::get()->addWidget(name, widget);
  }
}

UIWidget *createWidget(const std::string &paramName) {
  auto type = strParam(paramName + ".type");
  if (type == "TextWidget") {
    return new TextWidget(paramName);
  } else if (type == "TextureWidget") {
    return new TextureWidget(paramName);
  } else {
    invariant(false, "No widget found for type " + type);
  }
}



UIWidget::UIWidget()
  : clickable_(false),
    pos_(-1.f), size_(0.f) {
}

UIWidget* UIWidget::setClickable() {
  clickable_ = true;
  return this;
}

bool UIWidget::handleClick(const glm::vec2 &pos) {
  if (!onClickListener_) {
    return false;
  }
  return onClickListener_(pos);
}

UIWidget * UIWidget::setOnClickListener(UIWidget::OnClickListener l) {
  onClickListener_ = l;
  return this;
}


SizedWidget::SizedWidget(const std::string &name) {
  size_ = vec2Param(name + ".dim");
  center_ = UI::convertUIPos(vec2Param(name + ".pos")) + size_/2.f;
}

bool SizedWidget::isClick(const glm::vec2 &pos) const {
  return isClickable() && pointInBox(pos, center_, size_, 0.f);
}

TextureWidget::TextureWidget(const std::string &name) 
  : SizedWidget(name),
    texName_(strParam(name + ".texture")) {
}

void TextureWidget::render(float dt) {
  auto tex = ResourceManager::get()->getTexture(texName_);
  drawTextureCenter(getCenter(), getSize(), tex);
}

TextWidget::TextWidget(const std::string &name)
  : SizedWidget(name),
    height_(fltParam(name + ".fontHeight")),
    bgcolor_(vec4Param(name + ".bgcolor")) {
  if (hasParam(name + ".text")) {
    setText(strParam(name + ".text"));
  }
}

TextWidget *TextWidget::setTextFunc(const TextFunc &func) {
  textFunc_ = func;
  return this;
}

TextWidget *TextWidget::setText(const std::string &text) {
  text_ = text;
  return this;
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
