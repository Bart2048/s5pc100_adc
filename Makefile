# ������ӡ
$(info KERNELRELEASE=$(KERNELRELEASE))

# ����KERNELRELEASE(��Makefile���õ����ı���)�Ƿ���ֵ�ж��Ǳ�make�������ִ�У����Ǳ��ں˵���Makefile����
ifeq ($(KERNELRELEASE),)
# ��make�������ִ��


# $(shell pwd) ����Makefile��shell����ִ��pwd����
PWD := $(shell pwd)

# �ں˵�·��
KERNDIR := /home/ubuntu/work/linux-2.6.35-farsight/

all: 
# -C $(KERNDIR)   ���κ�����ǰ���л�make�ĵ�ǰ·�����ں�Դ����
# M=$(PWD)        �����ں���Makefile������ֻ����ָ��·���µ�ģ��
# modules         ֻ����ģ�飬�������ں�(zImage)
	$(MAKE) -C $(KERNDIR) M=$(PWD) modules
	
clean:
# ���ģ�����ɵ�һ���ļ�
	$(MAKE) -C $(KERNDIR) M=$(PWD) clean
	
else

# ���ں˵���Makefile����, ����ģ��
obj-m += adc.o

endif