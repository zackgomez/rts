#include "rts/Widgets.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/CommandWidget.h"
#include "rts/FontManager.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/UI.h"

namespace rts {

 // can read percentages as percent of resolution
glm::vec2 uiVec2Param(const std::string &name) {
  Json::Value raw = ParamReader::get()->getParam(name);
  invariant(raw.size() == 2, "bad param " + raw.toStyledString());

  auto res = Renderer::get()->getResolution();
  if (raw[0].isDouble() && raw[1].isDouble()) {
    return toVec2(raw) * res / 100.f;
  } else if (raw[0].isInt() && raw[1].isInt()) {
    return toVec2(raw);
  } else {
    invariant_violation("unexpected ui pos array element");
  }
}

glm::vec2 uiPosParam(const std::string &name) {
  auto res = Renderer::get()->getResolution();
  auto ret = uiVec2Param(name);
  ret.x = ret.x >= 0.f ? ret.x : res.x + ret.x;
  ret.y = ret.y >= 0.f ? ret.y : res.y + ret.y;

  return ret;
}

glm::vec2 uiSizeParam(const std::string &name) {
  Json::Value raw = ParamReader::get()->getParam(name);
  invariant(raw.size() == 2, "bad param " + raw.toStyledString());

  auto res = Renderer::get()->getResolution();
  if (raw[0].isDouble() && raw[1].isDouble()) {
    auto ret = toVec2(raw) * res / 100.f;
    float aspectFactor = (res.x / res.y);
    ret = aspectFactor > 0
      ? glm::vec2(ret.x / aspectFactor, ret.y)
      : glm::vec2(ret.x, ret.y / aspectFactor);
    return ret;
  } else if (raw[0].isInt() && raw[1].isInt()) {
    return toVec2(raw);
  } else {
    invariant_violation("unexpected ui pos array element");
  }
}

void createWidgets(UI *ui, const std::string &widgetGroupName) {
  Json::Value group = ParamReader::get()->getParam(widgetGroupName);
  for (auto it = group.begin(); it != group.end(); it++) {
    auto name = widgetGroupName + '.' + it.memberName();
    auto *widget = createWidget(name);
    if (widget) {
      ui->addWidget(name, widget);
    }
  }
}

UIWidget *createWidget(const std::string &paramName) {
  auto type = strParam(paramName + ".type");
  if (type == "TextWidget") {
    return new TextWidget(paramName);
  } else if (type == "TextureWidget") {
    return new TextureWidget(paramName);
  } else if (type == "CommandWidget") {
    return new CommandWidget(paramName);
  } else if (type == "CustomWidget") {
    return nullptr;
  } else {
    invariant_violation("No widget found for type " + type);
  }
}

ClickableWidget * ClickableWidget::setOnClickListener(ClickableWidget::OnClickListener l) {
  onClickListener_ = l;
  return this;
}

bool ClickableWidget::handleClick(const glm::vec2 &pos) {
  if (!isClick(pos) || !onClickListener_) {
    return false;
  }
  return onClickListener_(pos);
}

SizedWidget::SizedWidget(const std::string &name) {
  invariant(
      hasParam(name + ".pos") ^ hasParam(name + ".center"),
      "widgets should specify only pos or center");

  size_ = uiSizeParam(name + ".dim");
  if (hasParam(name + ".center")) {
    center_ = uiPosParam(name + ".center");
  } else {
    center_ = uiPosParam(name + ".pos") + size_/2.f;
  }
}

bool SizedWidget::isClick(const glm::vec2 &pos) const {
  return pointInBox(pos, center_, size_, 0.f);
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
