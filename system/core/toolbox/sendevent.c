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
输入设备的节点不仅在用户空间可读，而且是可写的，因此可以将将原始事件写入到节点中，从而实现模拟用户输入的功能。
sendevent工具的作用正是如此。其用法如下：
sendevent <节点路径> <类型><代码> <值>
可以看出，sendevent的输入参数与getevent的输出是对应的，只不过sendevent的参数为十进制。电源键的代码0x74的十进制为116
，因此可以通过快速执行如下两条命令实现点击电源键的效果：
adb shell sendevent /dev/input/event0 1 116 1 #按下电源键
adb shell sendevent /dev/input/event0 1 116 0 #抬起电源键
执行完这两条命令后，可以看到设备进入了休眠或被唤醒，与按下实际的电源键的效果一模一样。另外，执行这两条命令的时间间
隔便是用户按住电源键所保持的时间，所以如果执行第一条命令后迟迟不执行第二条，则会产生长按电源键的效果——关机对话框
出现了。很有趣不是么？输入设备节点在用户空间可读可写的特性为自动化测试提供了一条高效的途径。[1]
*/
int sendevent_main(int argc, char *argv[])//send event用起来比较繁琐，可以用input keyevent代替
{
    int fd;
    ssize_t ret;
    int version;
    struct input_event event;

    if(argc != 5) {
        fprintf(stderr, "use: %s device type code value\n", argv[0]);//使用方法:可以看到sendevent需要4个参数即：device，type，code，value。
        //这些值可以由input子系统定义，也可以从getevent里面获取，比如我们要模拟一次 BACK 事件，
        //根据前面的信息可知BACK的编码为0x9e即158，那我们输入如下命令即可模拟一次BACK键的按下和弹起：
        return 1;
    }
/*那我们输入如下命令即可模拟一次BACK键的按下和弹起：
# sendevent /dev/input/event1 1 158 1
# sendevent /dev/input/event1 1 158 0
device需要是支持该按键的设备这里是xx_ts；type为1表示是按键事件；value为1表示按下，为0表示弹起，
一次按键事件由按下和弹起两个操作组成。另外Android还在framework层提供了输入键值的工具input，这些我们后面再详细分析。

用senevent 模拟触屏事件
sendevent /dev/input/event1 0003 0000 0000015e    // ABS x 坐标
sendevent /dev/input/event1: 0003 0001 000000df    // ABS y 坐标
sendevent /dev/input/event1: 0001 014a 00000001   // BTN touch事件 值为1
sendevent /dev/input/event1: 0003 0018 00000000   // ABS pressure事件
sendevent /dev/input/event1: 0001 014a 00000000   // BTN touch事件 值为0
sendevent /dev/input/event1: 0000 0000 00000000   // sync事件
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
