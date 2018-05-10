#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <linux/input.h> // this does not compile
#include <errno.h>


// from <linux/input.h>

struct input_event {
	struct timeval time;
	__u16 type;
	__u16 code;
	__s32 value;
};

#define EVIOCGVERSION		_IOR('E', 0x01, int)			/* get driver version */
#define EVIOCGID		_IOR('E', 0x02, struct input_id)	/* get device ID */
#define EVIOCGKEYCODE		_IOR('E', 0x04, int[2])			/* get keycode */
#define EVIOCSKEYCODE		_IOW('E', 0x04, int[2])			/* set keycode */

#define EVIOCGNAME(len)		_IOC(_IOC_READ, 'E', 0x06, len)		/* get device name */
#define EVIOCGPHYS(len)		_IOC(_IOC_READ, 'E', 0x07, len)		/* get physical location */
#define EVIOCGUNIQ(len)		_IOC(_IOC_READ, 'E', 0x08, len)		/* get unique identifier */

#define EVIOCGKEY(len)		_IOC(_IOC_READ, 'E', 0x18, len)		/* get global keystate */
#define EVIOCGLED(len)		_IOC(_IOC_READ, 'E', 0x19, len)		/* get all LEDs */
#define EVIOCGSND(len)		_IOC(_IOC_READ, 'E', 0x1a, len)		/* get all sounds status */
#define EVIOCGSW(len)		_IOC(_IOC_READ, 'E', 0x1b, len)		/* get all switch states */

#define EVIOCGBIT(ev,len)	_IOC(_IOC_READ, 'E', 0x20 + ev, len)	/* get event bits */
#define EVIOCGABS(abs)		_IOR('E', 0x40 + abs, struct input_absinfo)		/* get abs value/limits */
#define EVIOCSABS(abs)		_IOW('E', 0xc0 + abs, struct input_absinfo)		/* set abs value/limits */

#define EVIOCSFF		_IOC(_IOC_WRITE, 'E', 0x80, sizeof(struct ff_effect))	/* send a force effect to a force feedback device */
#define EVIOCRMFF		_IOW('E', 0x81, int)			/* Erase a force effect */
#define EVIOCGEFFECTS		_IOR('E', 0x84, int)			/* Report number of effects playable at the same time */

#define EVIOCGRAB		_IOW('E', 0x90, int)			/* Grab/Release device */

// end <linux/input.h>


/*
�����豸�Ľڵ㲻�����û��ռ�ɶ��������ǿ�д�ģ���˿��Խ���ԭʼ�¼�д�뵽�ڵ��У��Ӷ�ʵ��ģ���û�����Ĺ��ܡ�
sendevent���ߵ�����������ˡ����÷����£�
sendevent <�ڵ�·��> <����><����> <ֵ>
���Կ�����sendevent�����������getevent������Ƕ�Ӧ�ģ�ֻ����sendevent�Ĳ���Ϊʮ���ơ���Դ���Ĵ���0x74��ʮ����Ϊ116
����˿���ͨ������ִ��������������ʵ�ֵ����Դ����Ч����
adb shell sendevent /dev/input/event0 1 116 1 #���µ�Դ��
adb shell sendevent /dev/input/event0 1 116 0 #̧���Դ��
ִ��������������󣬿��Կ����豸���������߻򱻻��ѣ��밴��ʵ�ʵĵ�Դ����Ч��һģһ�������⣬ִ�������������ʱ���
�������û���ס��Դ�������ֵ�ʱ�䣬�������ִ�е�һ�������ٳٲ�ִ�еڶ�����������������Դ����Ч�������ػ��Ի���
�����ˡ�����Ȥ����ô�������豸�ڵ����û��ռ�ɶ���д������Ϊ�Զ��������ṩ��һ����Ч��;����[1]
*/
int sendevent_main(int argc, char *argv[])//send event�������ȽϷ�����������input keyevent����
{
    int fd;
    ssize_t ret;
    int version;
    struct input_event event;

    if(argc != 5) {
        fprintf(stderr, "use: %s device type code value\n", argv[0]);//ʹ�÷���:���Կ���sendevent��Ҫ4����������device��type��code��value��
        //��Щֵ������input��ϵͳ���壬Ҳ���Դ�getevent�����ȡ����������Ҫģ��һ�� BACK �¼���
        //����ǰ�����Ϣ��֪BACK�ı���Ϊ0x9e��158���������������������ģ��һ��BACK���İ��º͵���
        return 1;
    }
/*�������������������ģ��һ��BACK���İ��º͵���
# sendevent /dev/input/event1 1 158 1
# sendevent /dev/input/event1 1 158 0
device��Ҫ��֧�ָð������豸������xx_ts��typeΪ1��ʾ�ǰ����¼���valueΪ1��ʾ���£�Ϊ0��ʾ����
һ�ΰ����¼��ɰ��º͵�������������ɡ�����Android����framework���ṩ�������ֵ�Ĺ���input����Щ���Ǻ�������ϸ������

��senevent ģ�ⴥ���¼�
sendevent /dev/input/event1 0003 0000 0000015e    // ABS x ����
sendevent /dev/input/event1: 0003 0001 000000df    // ABS y ����
sendevent /dev/input/event1: 0001 014a 00000001   // BTN touch�¼� ֵΪ1
sendevent /dev/input/event1: 0003 0018 00000000   // ABS pressure�¼�
sendevent /dev/input/event1: 0001 014a 00000000   // BTN touch�¼� ֵΪ0
sendevent /dev/input/event1: 0000 0000 00000000   // sync�¼�
*/
    fd = open(argv[1], O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "could not open %s, %s\n", argv[optind], strerror(errno));
        return 1;
    }
    if (ioctl(fd, EVIOCGVERSION, &version)) {
        fprintf(stderr, "could not get driver version for %s, %s\n", argv[optind], strerror(errno));
        return 1;
    }
    memset(&event, 0, sizeof(event));
    event.type = atoi(argv[2]);
    event.code = atoi(argv[3]);
    event.value = atoi(argv[4]);
    ret = write(fd, &event, sizeof(event));
    if(ret < (ssize_t) sizeof(event)) {
        fprintf(stderr, "write event failed, %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
