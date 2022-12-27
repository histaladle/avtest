#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    char *path="f:/video/tooyoung.mp4";

    av_register_all();
    avformat_network_init();

    avcodec_register_all();

    AVDictionary *opt=NULL;

    av_dict_set(&opt,"rtsp_transport","tcp",0);
    av_dict_set(&opt,"max_delay","500",0);

    AVFormatContext *ic=NULL;

    int re=0;
    re=avformat_open_input(&ic,path,NULL,&opt);
    int vi,ai;

    AVCodec *vdcd=NULL;
    AVCodec *adcd=NULL;
    AVCodecContext *vdcdctx=NULL;
    AVCodecContext *adcdctx=NULL;
    AVDictionary *vdcdopt=NULL;
    AVDictionary *adcdopt=NULL;
    if(re!=0)
    {
        char errbuf[1024];
        av_strerror(re,errbuf,sizeof(errbuf));
        qDebug() << "open failed:" << errbuf;
    }
    else
    {
        qDebug() << "openned"
                 << ic->duration/AV_TIME_BASE;
        avformat_find_stream_info(ic,&opt);
//        av_dump_format(ic,11,NULL,0);

        for(unsigned int i=0;i<ic->nb_streams;i++)
        {
            AVStream *str=ic->streams[i];
            if(str->codecpar->codec_type==AVMEDIA_TYPE_AUDIO)
            {
                ai=i;
                qDebug() << i << "audio:";
                qDebug() << "sample rate" << str->codecpar->sample_rate;
                qDebug() << "format" << str->codecpar->format;
                qDebug() << "channels" << str->codecpar->channels;
                qDebug() << "codec id" << str->codecpar->codec_id;
                qDebug() << "frame size" << str->codecpar->frame_size;
            }
            if(str->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
            {
                vi=i;
                qDebug() << i << "video:";
                qDebug() << "codec id" << str->codecpar->codec_id;
                qDebug() << "format" << str->codecpar->format;
                qDebug() << "width" << str->codecpar->width;
                qDebug() << "height" << str->codecpar->height;
                qDebug() << "fps" << str->avg_frame_rate.num
                         << str->avg_frame_rate.den;
            }
        }

        long long pos=ic->streams[vi]->duration*0.8;
        av_seek_frame(ic,vi,pos,
                      AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
        vdcd=avcodec_find_decoder(ic->streams[vi]->codecpar->codec_id);
        adcd=avcodec_find_decoder(ic->streams[ai]->codecpar->codec_id);

        if(!vdcd)
        {
            qDebug() << "video codec can't find";
            return;
        }
        qDebug() << "video codec found";
        if(!adcd)
        {
            qDebug() << "audio codec can't find";
            return;
        }
        qDebug() << "audio codec found";
        vdcdctx=avcodec_alloc_context3(vdcd);
        adcdctx=avcodec_alloc_context3(adcd);

        avcodec_parameters_to_context(vdcdctx,ic->streams[vi]->codecpar);
        avcodec_parameters_to_context(adcdctx,ic->streams[ai]->codecpar);


        avcodec_open2(adcdctx,NULL,&adcdopt);
        avcodec_open2(vdcdctx,NULL,&vdcdopt);
        AVPacket *pkt=av_packet_alloc();
        AVFrame *frm=av_frame_alloc();
        SwsContext *sctx=sws_alloc_context();
        SwrContext *rctx=swr_alloc();
        qDebug() << "debug3" << rctx;
//        rctx=swr_alloc_set_opts(
//                    rctx,
//                    av_get_default_channel_layout(2),
//                    AV_SAMPLE_FMT_S16,
//                    adcdctx->sample_rate,
//                    av_get_default_channel_layout(adcdctx->channels),
//                    adcdctx->sample_fmt,
//                    adcdctx->sample_rate,0,NULL);
        rctx=swr_alloc_set_opts(
                    rctx,
                    av_get_default_channel_layout(2),
                    AV_SAMPLE_FMT_S16,
                    ic->streams[ai]->codecpar->sample_rate,
                    av_get_default_channel_layout(adcdctx->channels),
                    (AVSampleFormat)ic->streams[ai]->codecpar->format,
                    ic->streams[ai]->codecpar->sample_rate,0,NULL);
        qDebug() << "debug1" << rctx;
        re = swr_init(rctx);
        qDebug() << "debug2";
        if(re!=0)
        {
            char errbuf[1024];
            av_strerror(re,errbuf,sizeof(errbuf));
            qDebug() << "swr init failed:" << errbuf;
            return;
        }
        unsigned char *dstpix=NULL;
        unsigned char *dstsamp=NULL;
        while(true)
        {
            re=av_read_frame(ic,pkt);
            if(re!=0)
            {
                break;
            }
            if(pkt->stream_index==vi)
            {
                qDebug() << "video";
                qDebug() << "pkt->size" << pkt->size;
                qDebug() << "pkt->pts"
                         << pkt->pts*1000
                            *r2d(ic->streams[vi]->time_base);
                qDebug() << "pkt->dts"
                         << pkt->dts*1000
                            *r2d(ic->streams[vi]->time_base);
                re=avcodec_send_packet(vdcdctx,pkt);
                if(re!=0)
                {
                    char errbuf[1024];
                    av_strerror(re,errbuf,sizeof(errbuf));
                    qDebug() << "send pkt failed:" << errbuf;
                    break;
                }
                av_packet_unref(pkt);
                do
                {
                    re=avcodec_receive_frame(vdcdctx,frm);
                    if(re!=0)
                    {
                        break;
                    }
                    qDebug() << "recv video frame"
                             << frm->format << frm->linesize[0];
                    sctx=sws_getContext(
                                frm->width,frm->height,
                                (AVPixelFormat)frm->format,
                                frm->width,frm->height,
                                AV_PIX_FMT_RGBA,
                                SWS_BILINEAR,NULL,NULL,NULL);
                    dstpix=new unsigned char[frm->width*frm->height*4];
                    unsigned char *dstpixes[2]={dstpix,NULL};
                    int dstw[2]={frm->width*4,0};
                    int dsth=sws_scale(sctx,frm->data,frm->linesize,
                                      0,frm->height,dstpixes,dstw);
                    qDebug() << "sws" << dsth;
                    delete[] dstpix;
                }while(!re);
            }
            else if(pkt->stream_index==ai)
            {
                qDebug() << "audio";
                re=avcodec_send_packet(adcdctx,pkt);
                if(re!=0)
                {
                    char errbuf[1024];
                    av_strerror(re,errbuf,sizeof(errbuf));
                    qDebug() << "send pkt failed:" << errbuf;
                    break;
                }
                av_packet_unref(pkt);
                do
                {
                    re=avcodec_receive_frame(adcdctx,frm);
                    if(re!=0)
                    {
                        break;
                    }
                    qDebug() << "recv audio frame"
                             << frm->format << frm->linesize[0]
                             << frm->pts;
                    dstsamp=new unsigned char[
                            frm->nb_samples*2*2];
                    unsigned char *dstsamps[2]={dstsamp,NULL};
                    int chn=swr_convert(
                                rctx,dstsamps,
                                frm->nb_samples,
                                (const unsigned char**)frm->data,
                                frm->nb_samples);
                    qDebug() << "swr" << chn;
                    delete[] dstsamp;
                }while(!re);
            }
        }
        av_packet_free(&pkt);
        av_frame_free(&frm);
    }

    if(ic)
    {
        avformat_close_input(&ic);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
