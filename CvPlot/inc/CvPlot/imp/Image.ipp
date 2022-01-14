// CvPlot - https://github.com/Profactor/cv-plot
// Copyright (c) 2019 by PROFACTOR GmbH - https://www.profactor.at/

#pragma once

#include <CvPlot/drawables/Image.h>
#include <CvPlot/Internal/util.h>
#include <opencv2/opencv.hpp>

namespace CvPlot {

namespace Imp{

CVPLOT_DEFINE_FUN
cv::Mat1b toMat1b(const cv::Mat& mat, double override_min = NAN, double override_max = NAN) {
    bool floating = mat.depth() == CV_32F || mat.depth() == CV_64F;
    cv::Mat mask = floating ? (mat == mat) : cv::Mat();
    double minVal, maxVal;
    if (std::isnan(override_min) || std::isnan(override_max) ){
        cv::minMaxLoc(mat, &minVal, &maxVal, nullptr, nullptr, mask);
    }
    if (!std::isnan(override_min) ) minVal = override_min;
    if (!std::isnan(override_max) ) maxVal = override_max;
    if(std::isinf(minVal) || std::isinf(maxVal)) {
        uint8_t finiteVal = std::isfinite(minVal) ? 0 : std::isfinite(maxVal) ? 255 : 127;
        cv::Mat1b mat1b(mat.size(), finiteVal);
        mat1b.setTo(0, mat == minVal);
        mat1b.setTo(255, mat == maxVal);
        return mat1b;
    }
    const double alpha = 255.0 / (maxVal - minVal);
    const double beta = -minVal * alpha;
    cv::Mat1b mat1b;
    mat.convertTo(mat1b, mat1b.type(), alpha, beta);
    return mat1b;
}

CVPLOT_DEFINE_FUN
cv::Mat3b toMat3b(const cv::Mat& mat, int code) {
    cv::Mat3b mat3b;
    if (!mat.empty()) {
        cv::cvtColor(mat, mat3b, code);
    }
    return mat3b;
}

CVPLOT_DEFINE_FUN
cv::Mat3b toBgr(const cv::Mat& mat, cv::Scalar nanColor, int colormap = -1, double override_min = NAN, double override_max = NAN) {
    switch (mat.type()) {
    case CV_8UC3:
        return mat;
    case CV_8UC4:
        return toMat3b(mat, cv::COLOR_BGRA2BGR);
    case CV_16S:
    case CV_16U:
    case CV_32S:{
        cv::Mat mat3b;
        if (colormap >= 0) {
            cv::Mat mat3b;
            cv::applyColorMap(toMat1b(mat, override_min ,override_max), mat3b, colormap);
            mat3b = toMat3b(mat3b, cv::COLOR_RGB2BGR);
        }else {
            mat3b = toMat3b(toMat1b(mat, override_min ,override_max), cv::COLOR_GRAY2BGR);
        }
        if (!std::isnan(override_min)){
            mat3b.setTo(nanColor, mat < override_min);
        }
        if (!std::isnan(override_max)){
            mat3b.setTo(nanColor, mat > override_max);
        }
        return mat3b;
    }
    case CV_32F:
    case CV_64F: {
        cv::Mat mat3b;
        if (colormap >= 0) {
            cv::applyColorMap(toMat1b(mat, override_min ,override_max), mat3b, colormap);
            mat3b = toMat3b(mat3b, cv::COLOR_RGB2BGR);
        }else {
            mat3b = toMat3b(toMat1b(mat, override_min ,override_max), cv::COLOR_GRAY2BGR);
        }
        mat3b.setTo(nanColor, mat != mat);
        if (!std::isnan(override_min)){
            mat3b.setTo(nanColor, mat < override_min);
        }
        if (!std::isnan(override_max)){
            mat3b.setTo(nanColor, mat > override_max);
        }
        return mat3b;
    }
    case CV_8UC1:
        if (colormap >= 0) {
            cv::Mat mat3b;
            cv::applyColorMap(mat, mat3b, colormap);
            return toMat3b(mat3b, cv::COLOR_RGB2BGR);
        }else {
            return toMat3b(mat, cv::COLOR_GRAY2BGR);
        }
    default:
        throw std::runtime_error("Image: mat type not supported");
    }
}

}

class Image::Impl {
public:
    cv::Mat _mat;
    cv::Mat3b _matBgr;
    cv::Rect2d _position;
    bool _autoPosition = true;
    int _interpolation = cv::INTER_AREA;
    cv::Scalar _nanColor = cv::Scalar::all(255);
    int _colormap = -1;

    cv::Mat _flippedMat;
    cv::Mat3b _flippedBgr;
    bool _flipVert = false;
    bool _flipHorz = false;
    double _override_min = NAN;
    double _override_max = NAN;

    void updateFlipped() {
        if (_mat.empty()) {
            return;
        }
        if (_flipVert || _flipHorz) {
            int code = (_flipVert&&_flipHorz) ? -1 : (_flipVert ? 0 : +1);
            if (_flippedMat.data == _mat.data) {
                _flippedMat = _flippedMat.clone();
            }
            if (_flippedBgr.data == _matBgr.data) {
                _flippedBgr = _matBgr.clone();
            }
            cv::flip(_mat, _flippedMat, code);
            cv::flip(_matBgr, _flippedBgr, code);
        }else {
            _flippedMat = _mat;
            _flippedBgr = _matBgr;
        }
    }
    void render(RenderTarget & renderTarget) {
        if (_mat.empty()) {
            return;
        }
        cv::Mat3b& innerMat = renderTarget.innerMat();

        auto tl = renderTarget.project(cv::Point2d(_position.x, _position.y));
        auto br = renderTarget.project(cv::Point2d(_position.x + _position.width, _position.y + _position.height));
        cv::Rect2d matRectDst(tl, br);

        bool flipVert = tl.y > br.y;
        bool flipHorz = tl.x > br.x;
        if (flipHorz != _flipHorz || flipVert != _flipVert) {
            _flipHorz = flipHorz;
            _flipVert = flipVert;
            updateFlipped();
        }
        Internal::paint(_flippedBgr, innerMat, matRectDst, _interpolation, _flippedMat);
    }
    void updateAutoPosition() {
        if (_autoPosition) {
            _position = cv::Rect2d(0, 0, _mat.cols, _mat.rows);
        }
    }
};

CVPLOT_DEFINE_FUN
Image::Image(const cv::Mat &mat) {
    setMat(mat);
    impl->_autoPosition = true;
}

CVPLOT_DEFINE_FUN
Image::Image(const cv::Mat &mat, const cv::Rect2d &position) {
    setMat(mat);
    setPosition(position);
}

CVPLOT_DEFINE_FUN
Image::~Image() {
}

CVPLOT_DEFINE_FUN
Image& Image::setMat(const cv::Mat & mat){
    impl->_mat = mat;
    impl->_matBgr = Imp::toBgr(impl->_mat, impl->_nanColor, impl->_colormap, impl->_override_min, impl->_override_max);   //ref-copy when bgr, clone otherwise
    impl->updateFlipped();
    impl->updateAutoPosition();
    return *this;
}

CVPLOT_DEFINE_FUN
cv::Mat Image::getMat() const{
    return impl->_mat;
}

CVPLOT_DEFINE_FUN
Image& Image::setPosition(const cv::Rect2d & position){
    impl->_position = position;
    impl->_autoPosition = false;
    return *this;
}

CVPLOT_DEFINE_FUN
cv::Rect2d Image::getPosition(){
    return impl->_position;
}

CVPLOT_DEFINE_FUN
Image & Image::setAutoPosition(bool autoPosition){
    impl->_autoPosition = autoPosition;
    impl->updateAutoPosition();
    return *this;
}

CVPLOT_DEFINE_FUN
bool Image::getAutoPosition()const{
    return impl->_autoPosition;
}

CVPLOT_DEFINE_FUN
Image & Image::setInterpolation(int interpolation){
    impl->_interpolation = interpolation;
    return *this;
}

CVPLOT_DEFINE_FUN
int Image::getInterpolation() const{
    return impl->_interpolation;
}

CVPLOT_DEFINE_FUN
Image& Image::setNanColor(cv::Scalar nanColor){
    if(nanColor==impl->_nanColor){
        return *this;
    }
    impl->_nanColor = nanColor;
    setMat(impl->_mat);
    return *this;
}

CVPLOT_DEFINE_FUN
cv::Scalar Image::getNanColor()const{
    return impl->_nanColor;
}

CVPLOT_DEFINE_FUN
Image& Image::setColormap(int colormap){
    if(colormap==impl->_colormap){
        return *this;
    }
    impl->_colormap = colormap;
    setMat(impl->_mat);
    return *this;
}

CVPLOT_DEFINE_FUN
int Image::getColormap()const{
    return impl->_colormap;
}

CVPLOT_DEFINE_FUN
void Image::render(RenderTarget & renderTarget){
    impl->render(renderTarget);
}

CVPLOT_DEFINE_FUN
bool Image::getBoundingRect(cv::Rect2d &rect) {
    if (impl->_mat.empty()) {
        return false;
    }
    rect = impl->_position;
    return true;
}

CVPLOT_DEFINE_FUN
Image& Image::setMinMaxOverride(double override_min, double override_max){
    if(impl->_override_min==override_min && impl->_override_max==override_max){
        return *this;
    }
    impl->_override_min = override_min;
    impl->_override_max = override_max;
    setMat(impl->_mat);
    return *this;
}

}
