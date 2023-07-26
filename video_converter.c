#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

AVCodecContext *output_codec_ctx;
AVFrame *output_frame;

void convert_video(char *input_file, char *output_file, char *in_fmt, char *out_fmt) {

  AVFormatContext *input_ctx = NULL;

  if (avformat_open_input(&input_ctx, input_file, NULL, NULL) != 0) {
    printf("Could not open input file\n");
    return;
  }

  int video_stream_idx = -1;

  for (int i = 0; i < input_ctx->nb_streams; i++) {
    if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_idx = i;
      break;
    }
  }

  if (video_stream_idx == -1) {
    printf("No video stream found\n");
    return;
  }

  AVStream *input_stream = input_ctx->streams[video_stream_idx];
  AVCodecParameters *input_codecpar = input_stream->codecpar;

  AVCodec *input_codec = (AVCodec*)avcodec_find_decoder(input_codecpar->codec_id);

  AVCodecContext *input_codec_ctx = avcodec_alloc_context3(NULL);
  avcodec_open2(input_codec_ctx, input_codec, NULL);
  avcodec_parameters_to_context(input_codec_ctx, input_codecpar);

  AVFormatContext *output_ctx;
  avformat_alloc_output_context2(&output_ctx, NULL, out_fmt, output_file);

  AVStream *output_stream = avformat_new_stream(output_ctx, input_codec);

  AVCodecParameters *output_codecpar = avcodec_parameters_alloc();
  avcodec_parameters_copy(output_codecpar, input_codecpar);

  output_stream->codecpar = output_codecpar;

  output_codec_ctx = avcodec_alloc_context3(NULL);
  avcodec_parameters_to_context(output_codec_ctx, output_codecpar);

  avio_open(&output_ctx->pb, output_file, AVIO_FLAG_WRITE);
  printf("Input WxH: %d x %d \n", input_codec_ctx->width, input_codec_ctx->height);
  printf("Input pixel format: %d \n", input_codec_ctx->pix_fmt);
  printf("SWS Params: \n");
  printf("W: %d \n", input_codec_ctx->width);
  printf("H: %d \n", input_codec_ctx->height);

  struct SwsContext *rescale_ctx = sws_getContext(input_codec_ctx->width,
                                                  input_codec_ctx->height,
                                                  input_codec_ctx->pix_fmt,
                                                  output_codec_ctx->width,
                                                  output_codec_ctx->height,
                                                  output_codec_ctx->pix_fmt,
                                                  SWS_BICUBIC,
                                                  NULL, NULL, NULL);

  output_frame = av_frame_alloc();

  int frame_idx = 0;
  AVPacket *packet = av_packet_alloc();


  while (av_read_frame(input_ctx, packet) >= 0) {

    if (packet->stream_index != video_stream_idx) {
      continue;
    }

    AVFrame *input_frame = av_frame_alloc();

    avcodec_send_packet(input_codec_ctx, packet);
    avcodec_receive_frame(input_codec_ctx, input_frame);

    sws_scale(rescale_ctx,
              (const uint8_t* const*)input_frame->data,
              input_frame->linesize,
              0,
              input_codec_ctx->height,
              output_frame->data,
              output_frame->linesize);

    output_frame->pts = frame_idx++;
    avcodec_send_frame(output_codec_ctx, output_frame);

    av_frame_free(&input_frame);
    av_packet_unref(packet);
  }

  av_packet_free(&packet);

  avcodec_close(input_codec_ctx);
  avformat_close_input(&input_ctx);

  av_write_trailer(output_ctx);
  avio_close(output_ctx->pb);
  avformat_free_context(output_ctx);

  av_frame_free(&output_frame);
  sws_freeContext(rescale_ctx);



}

int main(int argc, char *argv[]) {

  if (argc < 5) {
    printf("Usage: %s <input> <output> <in_fmt> <out_fmt>\n", argv[0]);
    return 1;
  }

  convert_video(argv[1], argv[2], argv[3], argv[4]);

  return 0;
}
