#pragma once

#include <QtCore/QtGlobal>

#define PROTOCOL_CODEC_NAMESPACE_BEGIN  namespace protocol_codec {
#define PROTOCOL_CODEC_NAMESPACE_END    }
#define PROTOCOL_CODEC_USING_NAMESPACE using namespace protocol_codec;

#ifndef PROTOCOL_CODEC_EXPORT
#  ifdef PROTOCOL_CODEC_STATIC
#    define PROTOCOL_CODEC_EXPORT
#  else
#    ifdef PROTOCOL_CODEC_LIBRARY
#      define PROTOCOL_CODEC_EXPORT Q_DECL_EXPORT
#    else
#      define PROTOCOL_CODEC_EXPORT Q_DECL_IMPORT
#    endif
#  endif
#endif