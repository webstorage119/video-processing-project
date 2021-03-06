# CMakeLists.txt in CodecUtils dir

INCLUDE_DIRECTORIES(
${vppIncludes}
${vppUtilIncludes}
)

# Adds a library called Shared (libRtvcCodecUtils.a under Linux)
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_BUILD_FOR_SHORT")

SET(CODEC_UTIL_HDRS
BitStreamBase.h
BitStreamBaseMSB.h
BitStreamReader.h
BitStreamReaderMSB.h
BitStreamWriter.h
BitStreamWriterMSB.h
BlockH264.h
CAVLCH264Impl.h
CAVLCH264Impl2.h
CodedBlkPatternH264VlcDecoder.h
CodedBlkPatternH264VlcEncoder.h
CoeffTokenH264VlcDecoder.h
CoeffTokenH264VlcEncoder.h
ExpGolombSignedVlcDecoder.h
ExpGolombSignedVlcEncoder.h
ExpGolombTruncVlcDecoder.h
ExpGolombTruncVlcEncoder.h
ExpGolombUnsignedVlcDecoder.h
ExpGolombUnsignedVlcEncoder.h
FastForward4x4ITImpl1.h
FastForward4x4ITImpl2.h
FastForward4x4On16x16ITImpl1.h
FastForwardDC2x2ITImpl1.h
FastForwardDC4x4ITImpl1.h
FastInverse4x4ITImpl1.h
FastInverse4x4On16x16ITImpl1.h
FastInverseDC2x2ITImpl1.h
FastInverseDC4x4ITImpl1.h
H264MotionVectorPredictorImpl1.h
IBitStreamReader.h
IBitStreamWriter.h
ICodecInnerAccess.h
ICodecv2.h
IContextAwareRunLevelCodec.h
IForwardTransform.h
IInverseTransform.h
IMotionCompensator.h
IMotionEstimator.h
IMotionVectorPredictor.h
IRunLengthCodec.h
IStreamHeaderReader.h
IVlcDecoder.h
IVlcEncoder.h
MacroBlockH264.h
MotionCompensatorH264ImplStd.h
MotionEstimatorH264ImplMultires.h
MotionEstimatorH264ImplMultiresCross.h
MotionEstimatorH264ImplMultiresCrossVer2.h
NalHeaderH264.h
PicParamSetH264.h
PrefixH264VlcDecoderImpl1.h
PrefixH264VlcEncoderImpl1.h
RunBeforeH264VlcDecoder.h
RunBeforeH264VlcEncoder.h
SeqParamSetH264.h
SliceHeaderH264.h
TotalZeros2x2H264VlcDecoder.h
TotalZeros2x2H264VlcEncoder.h
TotalZeros2x4H264VlcDecoder.h
TotalZeros2x4H264VlcEncoder.h
TotalZeros4x4H264VlcDecoder.h
TotalZeros4x4H264VlcEncoder.h
VectorStructList.h
)

SET(CODEC_UTILS_SRCS
BitStreamReader.cpp
BitStreamReaderMSB.cpp
BitStreamWriter.cpp
BitStreamWriterMSB.cpp
BlockH264.cpp
CAVLCH264Impl.cpp
CAVLCH264Impl2.cpp
CodedBlkPatternH264VlcDecoder.cpp
CodedBlkPatternH264VlcEncoder.cpp
#CodeFragmentForH264BlockLayerTest.cpp
#CodeFragmentForH264MacroBlockLayerTest.cpp
CoeffTokenH264VlcDecoder.cpp
CoeffTokenH264VlcEncoder.cpp
ExpGolombUnsignedVlcDecoder.cpp
ExpGolombUnsignedVlcEncoder.cpp
FastForward4x4ITImpl1.cpp
FastForward4x4ITImpl2.cpp
FastForward4x4On16x16ITImpl1.cpp
FastForwardDC2x2ITImpl1.cpp
FastForwardDC4x4ITImpl1.cpp
FastInverse4x4ITImpl1.cpp
FastInverse4x4On16x16ITImpl1.cpp
FastInverseDC2x2ITImpl1.cpp
FastInverseDC4x4ITImpl1.cpp
MacroBlockH264.cpp
MotionCompensatorH264ImplStd.cpp
MotionEstimatorH264ImplMultires.cpp
MotionEstimatorH264ImplMultiresCross.cpp
MotionEstimatorH264ImplMultiresCrossVer2.cpp
NalHeaderH264.cpp
PicParamSetH264.cpp
RunBeforeH264VlcDecoder.cpp
RunBeforeH264VlcEncoder.cpp
SeqParamSetH264.cpp
SliceHeaderH264.cpp
TotalZeros2x2H264VlcDecoder.cpp
TotalZeros2x2H264VlcEncoder.cpp
TotalZeros2x4H264VlcDecoder.cpp
TotalZeros2x4H264VlcEncoder.cpp
TotalZeros4x4H264VlcDecoder.cpp
TotalZeros4x4H264VlcEncoder.cpp
VectorStructList.cpp
)

ADD_LIBRARY( RtvcCodecUtils STATIC ${CODEC_UTILS_SRCS} ${CODEC_UTIL_HDRS})

if (CMAKE_BUILD_TYPE STREQUAL Debug)
SET_TARGET_PROPERTIES(RtvcCodecUtils PROPERTIES DEBUG_POSTFIX "D" )
message("CodecUtils Debug Build")
else (CMAKE_BUILD_TYPE STREQUAL Debug)
SET_TARGET_PROPERTIES(RtvcCodecUtils PROPERTIES DEBUG_POSTFIX "")
message("CodecUtils Release Build")
endif (CMAKE_BUILD_TYPE STREQUAL Debug)

