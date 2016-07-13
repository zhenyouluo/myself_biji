#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
 #include <asm/types.h>
#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {
        void * start;
        size_t length;
};

static char * dev_name = "/dev/video1";//����ͷ�豸��
static int fd = -1;
struct buffer * buffers = NULL;
static unsigned int n_buffers = 0;

FILE *file_fd;
static unsigned long file_length;
static unsigned char *file_name;
//////////////////////////////////////////////////////
//��ȡһ֡����
//////////////////////////////////////////////////////
 static int read_frame (void)
{
    struct v4l2_buffer buf;
    unsigned int i;

    CLEAR (buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
/*8.��������ȡ���Ѳɼ����ݵ�֡���壬ȡ��ԭʼ�ɼ����ݡ�VIDIOC_DQBUF*/
    int ff = ioctl (fd, VIDIOC_DQBUF, &buf);
    if(ff<0)
        printf("failture\n"); //���вɼ���֡����

    assert (buf.index < n_buffers);
       printf ("buf.index dq is %d,\n",buf.index);

    fwrite(buffers[buf.index].start, buffers[buf.index].length, 1, file_fd); //����д���ļ���
	fflush(file_fd);
//    CLEAR (buf);
/*9.���������������β,��������ѭ���ɼ���VIDIOC_QBUF*/
    ff=ioctl (fd, VIDIOC_QBUF, &buf); //�ٽ�������
    if(ff<0)//�����ݴӻ����ж�ȡ����
        printf("failture VIDIOC_QBUF\n");
    return 1;
}
 int main (int argc,char ** argv)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    unsigned int i;
    enum v4l2_buf_type type;

    file_fd = fopen("test-mmap.jpg", "w");//ͼƬ�ļ���
/*1.���豸�ļ��� int fd=open(��/dev/video0��,O_RDWR);*********/
    fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);//���豸

/*2.ȡ���豸��capability�������豸����ʲô���ܣ������Ƿ������Ƶ����,������Ƶ��������ȡ�VIDIOC_QUERYCAP,struct v4l2_capability*/
    int ff=ioctl (fd, VIDIOC_QUERYCAP, &cap);//��ȡ����ͷ����
    if(ff<0)
        printf("failture VIDIOC_QUERYCAP\n");

/*3.������Ƶ����ʽ��֡��ʽ����ʽ����PAL��NTSC��֡�ĸ�ʽ��������Ⱥ͸߶ȵȡ�*/
        struct v4l2_fmtdesc fmt1;
        int ret;
        memset(&fmt1, 0, sizeof(fmt1));
        fmt1.index = 0;
        fmt1.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//��ȡ��ǰ����֧�ֵ���Ƶ��ʽ
        while ((ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmt1)) == 0)
        {
              fmt1.index++;
              printf("{ pixelformat = '%c%c%c%c', description = '%s' }\n",fmt1.pixelformat & 0xFF, (fmt1.pixelformat >> 8) & 0xFF,(fmt1.pixelformat >> 16) & 0xFF, (fmt1.pixelformat >> 24) & 0xFF,fmt1.description);
        }
    //֡�ĸ�ʽ�������ȣ��߶ȵ�
    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //���������ͣ�������Զ��V4L2_BUF_TYPE_VIDEO_CAPTURE
    fmt.fmt.pix.width = 640;//��������16�ı���
    fmt.fmt.pix.height = 480;////�ߣ�������16�ı���
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;//��Ƶ���ݴ洢����//V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_YVU420;//V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    //���õ�ǰ������Ƶ�����ʽ
    ff = ioctl (fd, VIDIOC_S_FMT, &fmt);
    if(ff<0)
        printf("failture VIDIOC_S_FMT\n");
    //����ͼƬ��С
    file_length = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;

/*4.����������֡���壬һ�㲻����5����struct v4l2_requestbuffers*/
    struct v4l2_requestbuffers req;
    CLEAR (req);
    req.count = 1;//����������Ҳ����˵�ڻ�������ﱣ�ֶ�������Ƭ
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;//��V4L2_MEMORY_USERPTR

    ff = ioctl (fd, VIDIOC_REQBUFS, &req); //���뻺�壬count�����������
    if(ff<0)
        printf("failture VIDIOC_REQBUFS\n");
    if (req.count < 1)
        printf("Insufficient buffer memory\n");

    buffers = (struct buffer*)calloc (req.count, sizeof (*buffers));//�ڴ��н�����Ӧ�ռ�
/*5.�����뵽��֡����ӳ�䵽�û��ռ䣬�����Ϳ���ֱ�Ӳ����ɼ�����֡�ˣ�������ȥ���ơ�mmap*/
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        struct v4l2_buffer buf; //�����е�һ֡
        CLEAR (buf);
           buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
           buf.memory = V4L2_MEMORY_MMAP;
           buf.index = n_buffers;
        //��VIDIOC_REQBUFS�з�������ݻ���ת���������ַ
           if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf)) //ӳ���û��ռ�
                printf ("VIDIOC_QUERYBUF error\n");

           buffers[n_buffers].length = buf.length;
           buffers[n_buffers].start = mmap (NULL /* start anywhere */,buf.length,PROT_READ | PROT_WRITE /* required */,MAP_SHARED /* recommended */,fd, buf.m.offset);//ͨ��mmap����ӳ���ϵ,����ӳ��������ʼ��ַ
    if (MAP_FAILED == buffers[n_buffers].start)
            printf ("mmap failed\n");
        }
/*6.�����뵽��֡����ȫ������У��Ա��Ųɼ���������.VIDIOC_QBUF,struct v4l2_buffer*/
    for (i = 0; i < n_buffers; ++i)
    {
           struct v4l2_buffer buf;
           CLEAR (buf);

           buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
         buf.memory = V4L2_MEMORY_MMAP;
           buf.index = i;
            //�����ݴӻ����ж�ȡ����
           if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))//���뵽�Ļ�������ж�
                printf ("VIDIOC_QBUF failed\n");
    }
                
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
/*7.��ʼ��Ƶ�Ĳɼ���VIDIOC_STREAMON*/
    if (-1 == ioctl (fd, VIDIOC_STREAMON, &type)) //��ʼ��׽ͼ������
           printf ("VIDIOC_STREAMON failed\n");

	for(;;){
     //��һ���漰���첽IO
    
           fd_set fds;
           struct timeval tv;
           int r;

           FD_ZERO (&fds);//��ָ���ļ������������
           FD_SET (fd, &fds);//���ļ������������и��µ��ļ�������

           /* Timeout. */
           tv.tv_sec = 2;
           tv.tv_usec = 0;

           r = select (fd + 1, &fds, NULL, NULL, &tv);//�ж��Ƿ�ɶ���������ͷ�Ƿ�׼���ã���tv�Ƕ�ʱ

           if (-1 == r)
        {
                if (EINTR == errno)
                     continue;
                printf ("select err\n");
                }
           if (0 == r) {
                fprintf (stderr, "select timeout\n");
                exit (EXIT_FAILURE);
                }
			
           if (read_frame ())//����ɶ���ִ��read_frame ()������������ѭ��
               break;
		//	 continue;

			  
	//	   read_frame();
    }

    unmap:
    for (i = 0; i < n_buffers; ++i)
       if (-1 == munmap (buffers->start, buffers->length))
            printf ("munmap error");
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 /*10.ֹͣ��Ƶ�Ĳɼ���VIDIOC_STREAMOFF*/
        if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type))
                printf("VIDIOC_STREAMOFF");
/*11.�ر���Ƶ�豸��close(fd);*/
    close (fd);
    fclose (file_fd);
    exit (EXIT_SUCCESS);
    return 0;
}
