// Simplest FFmpeg Muxer.cpp : �������̨Ӧ�ó������ڵ㡣
//

/**
* ��򵥵Ļ��� FFmpeg ������Ƶ������
* Simplest FFmpeg Muxer
*
* Դ����
* ������ Lei Xiaohua
* leixiaohua1020@126.com
* �й���ý��ѧ/���ֵ��Ӽ���
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* �޸ģ�
* ���ĳ� Liu Wenchen
* 812288728@qq.com
* ���ӿƼ���ѧ/������Ϣ
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* ��������Խ���Ƶ��������Ƶ���������һ�ַ�װ��ʽ�С�
* �����н� AAC �������Ƶ������ H.264 �������Ƶ��������� MPEG2TS ��װ��ʽ���ļ���
* ��Ҫע����Ǳ����򲢲��ı�����Ƶ�ı����ʽ��
*
* This software mux a video bitstream and a audio bitstream together into a file.
* In this example, it mux a H.264 bitstream (in MPEG2TS) and
* a AAC bitstream file together into MP4 format file.
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

// �������'fopen': This function or variable may be unsafe.Consider using fopen_s instead.
#pragma warning(disable:4996)

// ��������޷��������ⲿ���� __imp__fprintf���÷����ں��� _ShowError �б�����
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// ��������޷��������ⲿ���� __imp____iob_func���÷����ں��� _ShowError �б�����
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
}

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavformat/avformat.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif

/*
FIX: H.264 in some container formats (FLV, MP4, MKV etc.)
need "h264_mp4toannexb" bitstream filter (BSF).
1. Add SPS,PPS in front of IDR frame
2. Add start code ("0,0,0,1") in front of NALU
H.264 in some containers (such as MPEG2TS) doesn't need this BSF.
*/

// 1: Use H.264 Bitstream Filter 
#define USE_H264BSF 0


/*
FIX:AAC in some container formats (FLV, MP4, MKV etc.)
need "aac_adtstoasc" bitstream filter (BSF).
*/

// 1: Use AAC Bitstream Filter 
#define USE_AACBSF 0


int main(int argc, char* argv[])
{
	AVOutputFormat *ofmt = NULL;
	// Input AVFormatContext
	AVFormatContext *ifmt_ctx_video = NULL, *ifmt_ctx_audio = NULL;
	// Output AVFormatContext
	AVFormatContext *ofmt_ctx = NULL;
	AVPacket pkt;

	int ret;
	int videoindex_v = -1, videoindex_out = -1;
	int audioindex_a = -1, audioindex_out = -1;
	int frame_index = 0;
	int64_t cur_pts_video = 0, cur_pts_audio = 0;

	// Input file URL
	const char *in_filename_video = "cuc_ieschool.ts";
	const char *in_filename_audio = "huoyuanjia.mp3";
	// Output file URL
	const char *out_filename = "cuc_ieschool.mp4";

	av_register_all();

	// ����
	ret = avformat_open_input(&ifmt_ctx_video, in_filename_video, 0, 0);
	if (ret < 0)
	{
		printf("Could not open input video file.\n");
		goto end;
	}
	ret = avformat_find_stream_info(ifmt_ctx_video, 0);
	if (ret < 0)
	{
		printf("Failed to retrieve input stream information.\n");
		goto end;
	}

	ret = avformat_open_input(&ifmt_ctx_audio, in_filename_audio, 0, 0);
	if (ret < 0)
	{
		printf("Could not open input audio file.\n");
		goto end;
	}
	ret = avformat_find_stream_info(ifmt_ctx_audio, 0);
	if (ret < 0)
	{
		printf("Failed to retrieve input stream information.\n");
		goto end;
	}

	// Print some input information
	printf("\n=========== Input Information ==========\n");
	av_dump_format(ifmt_ctx_video, 0, in_filename_video, 0);
	av_dump_format(ifmt_ctx_audio, 0, in_filename_audio, 0);

	// ���
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx)
	{
		printf("Could not create output context.\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;

	for (size_t i = 0; i < ifmt_ctx_video->nb_streams; i++)
	{
		// Create output AVStream according to input AVStream
		if (ifmt_ctx_video->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			AVStream *in_stream = ifmt_ctx_video->streams[i];
			AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
			videoindex_v = i;

			if (out_stream == NULL)
			{
				printf("Failed allocating output stream.\n");
				ret = AVERROR_UNKNOWN;
				goto end;
			}

			videoindex_out = out_stream->index;

			// Copy the settings of AVCodecContext
			ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
			if (ret < 0)
			{
				printf("Failed to copy context from input to output stream codec context.\n");
				goto end;
			}
			out_stream->codec->codec_tag = 0;

			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			{
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
			break;
		}
	}

	for (size_t i = 0; i < ifmt_ctx_audio->nb_streams; i++)
	{
		// Create output AVStream according to input AVStream
		if (ifmt_ctx_audio->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			AVStream *in_stream = ifmt_ctx_audio->streams[i];
			AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
			audioindex_a = i;

			if (out_stream == NULL)
			{
				printf("Failed allocating output stream.\n");
				ret = AVERROR_UNKNOWN;
				goto end;
			}

			audioindex_out = out_stream->index;

			// Copy the settings of AVCodecContext
			ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
			if (ret < 0)
			{
				printf("Failed to copy context from input to output stream codec context.\n");
				goto end;
			}
			out_stream->codec->codec_tag = 0;

			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			{
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
			break;
		}
	}

	// Print some output information
	printf("\n============== Output Information ============\n");
	av_dump_format(ofmt_ctx, 0, out_filename, 1);

	// Open output file
	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			printf("Could not open output file '%s'.\n", out_filename);
			goto end;
		}
	}

	// Write file header
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0)
	{
		printf("Error occurred when opening output file.\n");
		goto end;
	}
#if USE_H264BSF
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
#if USE_AACBSF
	AVBitStreamFilterContext* aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
#endif

	while (1)
	{
		AVFormatContext *ifmt_ctx;
		AVStream *in_stream, *out_stream;

		int stream_index = 0;

		ret = av_compare_ts(cur_pts_video, ifmt_ctx_video->streams[videoindex_v]->time_base,
			cur_pts_audio, ifmt_ctx_audio->streams[audioindex_a]->time_base);
		if (ret <= 0)
		{
			ifmt_ctx = ifmt_ctx_video;
			stream_index = videoindex_out;

			// ��ȡһ�� AVPacket
			if (av_read_frame(ifmt_ctx, &pkt) < 0)
			{
				break;
			}

			do
			{
				in_stream = ifmt_ctx->streams[pkt.stream_index];
				out_stream = ofmt_ctx->streams[stream_index];

				if (pkt.stream_index == videoindex_v)
				{
					// FIX��No PTS (Example: Raw H.264)
					// Simple Write PTS
					if (pkt.pts == AV_NOPTS_VALUE)
					{
						// Write PTS
						AVRational time_base1 = in_stream->time_base;
						// Duration between 2 frames (us)
						int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
						// Parameters
						pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
						pkt.dts = pkt.pts;
						pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
						frame_index++;
					}

					cur_pts_video = pkt.pts;
					break;
				}
			} while (av_read_frame(ifmt_ctx, &pkt) >= 0);
		}
		else
		{
			ifmt_ctx = ifmt_ctx_audio;
			stream_index = audioindex_out;

			// ��ȡһ�� AVPacket
			if (av_read_frame(ifmt_ctx, &pkt) < 0)
			{
				break;
			}

			do
			{
				in_stream = ifmt_ctx->streams[pkt.stream_index];
				out_stream = ofmt_ctx->streams[stream_index];

				if (pkt.stream_index == audioindex_a)
				{
					// FIX��No PTS
					// Simple Write PTS
					if (pkt.pts == AV_NOPTS_VALUE)
					{
						// Write PTS
						AVRational time_base1 = in_stream->time_base;
						// Duration between 2 frames (us)
						int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
						// Parameters
						pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
						pkt.dts = pkt.pts;
						pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
						frame_index++;
					}

					cur_pts_audio = pkt.pts;
					break;
				}
			} while (av_read_frame(ifmt_ctx, &pkt) >= 0);
		}

		// FIX:Bitstream Filter
#if USE_H264BSF
		av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
#if USE_AACBSF
		av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
		// ת�� PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base,
			out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base,
			out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index = stream_index;

		printf("Write 1 packet to output file. size:%5d\tpts:%lld\n", pkt.size, pkt.pts);

		// Write
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0)
		{
			printf("Error muxing packet.\n");
			break;
		}


		av_free_packet(&pkt);
	}

	// Write file trailer
	av_write_trailer(ofmt_ctx);

#if USE_H264BSF
	av_bitstream_filter_close(h264bsfc);
#endif
#if USE_AACBSF
	av_bitstream_filter_close(aacbsfc);
#endif

end:
	avformat_close_input(&ifmt_ctx_video);
	avformat_close_input(&ifmt_ctx_audio);

	// Close output
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
	{
		avio_close(ofmt_ctx->pb);
	}
	avformat_free_context(ofmt_ctx);

	if (ret < 0 && ret != AVERROR_EOF)
	{
		printf("Error occurred.\n");
		return -1;
	}

	system("pause");
	return 0;
}
