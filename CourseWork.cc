#include "declarations.h" //���������� ���������
#include "roby.h"//������� ��� ������
int coid_roby;//connection id
int chidWF;//channel id WF
int stateW;//��- ��- ������� ��� W
int stateF;//��- ��- ������� ��� F
pid_t pid;//Process ID
int displayWF;//������������� ������ ����� � ������� displayWF
int x,y,z;//�������� ��� ��������� X,Y,Z
//��������� ��� ������������� ���������
struct MESSAGE {
	unsigned char type;
	unsigned int buf;
};

struct MESSAGE Amsg; // ���������� ��������� ��� ������ � ������� �
struct MESSAGE Cmsg; // ���������� ��������� ��� ������ � ������� �
int f_cnt; //������� ���������� f
int w_cnt; //������� ���������� w

//������� ��������� � ����� ���������������� �����
int raw(int fd) {
	struct termios termios_p;
	if (tcgetattr(fd, &termios_p))
		return (-1);
	termios_p.c_cc[VMIN] = 1;
	termios_p.c_cc[VTIME] = 0;
	termios_p.c_lflag &= ~(ECHO | ICANON | ISIG | ECHOE | ECHOK | ECHONL);
	termios_p.c_oflag &= ~(OPOST);
	return (tcsetattr(fd, TCSADRAIN, &termios_p));
}

//������� ��������� � ����� �������������� �����
int unraw(int fd) {
	struct termios termios_p;
	if (tcgetattr(fd, &termios_p))
		return (-1);
	termios_p.c_lflag |= (ECHO | ICANON | ISIG | ECHOE | ECHOK | ECHONL);
	termios_p.c_oflag |= (OPOST);
	return (tcsetattr(fd, TCSADRAIN, &termios_p));
}

//����� ����������� ��������� XYZ
void* DisplayXYZ(void* arg) {
	int chid;//������������� ������ ����� Roby->DisplayXYZ
	x = -1, y = -1, z = -1;//��������� ��� ����������� ���������
	struct MESSAGE msg;//��������� ��� ������������� ��������
	struct _pulse pulse;//��������� ��� ������ ��������� �� Roby
	chid = ChannelCreate(NULL);//�������� ����������������� ������
	msg.buf = chid;//���������� id ������ � ��������� �������������
	int st = 0;//������ �������� ���������
	for (int j = 4; j < 7; ++j) {//���� ������������� �������� XYZ
		msg.type = j;//��� ��������� ��� ���������������� �������
		printf("\nXYZ - before %d", j);
		st = MsgSend(coid_roby, &msg, sizeof(msg), 0, 0);//��������
		printf("\nXYZ - after %d, st = %d", j, st);
	}
	printf("\nXYZ - initialized");
	while (1) {//�������� ���� ������ ���������
		MsgReceivePulse(chid, &pulse, sizeof(pulse), NULL);//����� �������� �� Roby
		switch (pulse.code) {//������ ��������
		case B_X:
			x = pulse.value.sival_int;
			break;
		case -2://��� �������� �� Y ������
		case B_Y:
			y = pulse.value.sival_int;
			break;
		case B_Z:
			z = pulse.value.sival_int;
			break;
		}
		printf("\n\rx = %d y = %d z = %d ", x, y, z);//����� �������� ���������
		//��������� ����� ���������� ��� ��������� �������� � �������� ����������
		if (x == 0)
			Amsg.buf &= ~A_X_BACK;
		else if (x == 1024)
			Amsg.buf &= ~A_X_FORWARD;
		else if (y == 0)
			Amsg.buf &= ~A_Y_BACK;
		else if (y == 1024)
			Amsg.buf &= ~A_X_FORWARD;
		else if (z == 0)
			Amsg.buf &= ~A_Z_BACK;
		else if (z == 1024)
			Amsg.buf &= ~A_Z_FORWARD;
	}//end while
	pthread_exit(NULL);
}

//����� ����������� ��������� WF
void* DisplayWF(void *arg) {
	struct MESSAGE msg;//��������� ��� �������������
	struct _pulse pulse;//��������� ��� ���������� ���������
	struct _itimer it, old_it;//��������� ��� ���������� ������� W
	struct _itimer itF, old_itF;//��������� ��� ���������� ������� F
	//������������� ���������� ��������
	it.nsec = NSEC;//����� "�������" �������
	it.interval_nsec = INT_NSEC;//�������� ���������� ������� �������
	itF.nsec = NSEC;
	itF.interval_nsec = INT_NSECF;
	chidWF = ChannelCreate(0);//�������� ������ displayWF-Roby-Main
	printf("channel has chid = %d \n", chidWF);
	int coidW = ConnectAttach(0, pid, chidWF, 0, 0);//���������� ��� ������� ��������
	int coidF = ConnectAttach(0, pid, chidWF, 0, 0);
	struct sigevent eventW, eventF;//��������� �������
	//������� ��� �������� �������� ���������� ��������� �� ��������
	SIGEV_PULSE_INIT(&eventW,coidW,SIGEV_PULSE_PRIO_INHERIT,TIMER_W_COUNT,NULL);
	SIGEV_PULSE_INIT(&eventF,coidF,SIGEV_PULSE_PRIO_INHERIT,TIMER_F_COUNT,NULL);
	//�������� ��������-"�����������" �� "��������"-���������
	int timer_W = TimerCreate(CLOCK_REALTIME, &eventW);
	int timer_F = TimerCreate(CLOCK_REALTIME, &eventF);
	sleep(3);
	for (int i = 7; i < 9; i++) {//���� ������������� �������� WF
		msg.type = i;
		msg.buf = chidWF;
		printf("Init W sensor \n");
		MsgSend(coid_roby, &msg, sizeof(msg), 0, 0);
		printf("Init F sensor \n");
	}
	f_cnt = 500;//�������� ��������� ���������
	w_cnt = 1000;//� ��������� �����
	while (1) {
		MsgReceivePulse(chidWF, &pulse, sizeof(pulse), NULL);
		switch (pulse.code) {//������ ����������� ��������
		case TIMER_W_SET:
			//���� ���� �������� �� ���������� - ������ ������� (��� NSEC) , ����� - ������� (��� 0)
			it.nsec = Cmsg.buf & (C_W_BACK | C_W_FORWARD) ? NSEC : 0;
			TimerSettime(timer_W, NULL, &it, &old_it);//������ � ������ ����� ����������, ������ ��� nsec != 0
			break;
		case W_END:
			if (w_cnt != 2000) {//� ������ �������������
				w_cnt = 2000;//������������� ��������� �� ��������
				Cmsg.buf &= ~C_W_FORWARD;//��������� ���� �������� � ����������� �� ��������
			}
			stateW = 0;
			it.nsec = 0;
			TimerSettime(timer_W, NULL, &it, &old_it);//������ � ������ ����� ����������
			break;
		case W_BEGIN:
			if (w_cnt) {//� ������ �������������
				w_cnt = 0;//������������� ��������� �� ���������
				Cmsg.buf &= ~C_W_BACK;
			}
			stateW = 0;
			it.nsec = 0;//��� ��������� �������
			TimerSettime(timer_W, NULL, &it, &old_it);
			break;
		case TIMER_W_COUNT:
			w_cnt += w_cnt == 2001 || w_cnt == -1 ? 0 : stateW;
			break;
		case F_END:
			if (f_cnt != 1000) {
				f_cnt = 1000;
			}
			stateF = 0;
			itF.nsec = 0;
			TimerSettime(timer_F, NULL, &itF, &old_itF);
			break;
		case F_BEGIN:
			if (f_cnt != 0) {
				f_cnt = 0;
				Cmsg.buf &= ~C_F_BACK;
			}
			stateF = 0;
			itF.nsec = 0;
			TimerSettime(timer_F, NULL, &itF, &old_itF);
			break;
		case TIMER_F_SET:
			itF.nsec = Cmsg.buf & (C_F_BACK | C_F_FORWARD) ? NSEC : 0;
			TimerSettime(timer_F, NULL, &itF, &old_itF);
			break;
		case TIMER_F_COUNT:
			f_cnt += f_cnt == 1001 || f_cnt == -1 ? 0 : stateF;
			break;//��� ����������� ������������ ������ ����� ��������� ��������,
		};//��������� �� �������� �����, ����� ��������, ��� �� ��� �������� � ���� �����������
		printf("w = %d, f = %d\n\r", w_cnt, f_cnt);
	};
	pthread_exit(NULL);
}
//������� ������� ������� ������� � ����������
void control(int c3) {
	switch (c3) {//������ 3 ����� ���� F1-F12
	case 80://F1 X->
		Amsg.buf &= ~A_X_BACK;//�������� ��� � ��������������� �����������
		Amsg.buf = Amsg.buf ^ A_X_FORWARD;//������ ��������� ���� ����������. ������� �������
		break;
	case 81://F2 X<-
		Amsg.buf &= ~A_X_FORWARD;//�������� ��� � ��������������� �����������
		Amsg.buf = Amsg.buf ^ A_X_BACK;//������ ��������� ��������
		break;
	case 82://F3 Y->
		Amsg.buf &= ~A_Y_BACK;
		Amsg.buf = Amsg.buf ^ A_Y_FORWARD;
		break;
	case 83://F4 Y<-
		Amsg.buf &= ~A_Y_FORWARD;
		Amsg.buf = Amsg.buf ^ A_Y_BACK;
		break;
	case 84://F5 Z->
		Amsg.buf &= ~A_Z_BACK;
		Amsg.buf = Amsg.buf ^ A_Z_FORWARD;
		break;
	case 85://F6 Z<-
		Amsg.buf &= ~A_Z_FORWARD;
		Amsg.buf = Amsg.buf ^ A_Z_BACK;
		break;
	case 86://F7 F->
		Cmsg.buf &= ~C_F_BACK;
		Cmsg.buf = Cmsg.buf ^ C_F_FORWARD;
		stateF = Cmsg.buf & C_F_FORWARD ? 1 : 0;//�������������/������� ��������� �� F
		MsgSendPulse(displayWF, 10, TIMER_F_SET, 0);//���������� ������� ������ displayWF
		break;
	case 87://F8 F<-
		Cmsg.buf &= ~C_F_FORWARD;
		Cmsg.buf = Cmsg.buf ^ C_F_BACK;
		stateF = Cmsg.buf & C_F_BACK ? -1 : 0;//�������������/������� ��������� �� F
		MsgSendPulse(displayWF, 10, TIMER_F_SET, 0);//���������� ������� ������ displayWF
		break;
	case 88://F9 W->
		Cmsg.buf &= ~C_W_BACK;
		Cmsg.buf = Cmsg.buf ^ C_W_FORWARD;
		stateW = Cmsg.buf & C_W_FORWARD ? 1 : 0;
		MsgSendPulse(displayWF, 10, TIMER_W_SET, 0);
		break;
	case 89://F10 W<-
		Cmsg.buf &= ~C_W_FORWARD;
		Cmsg.buf = Cmsg.buf ^ C_W_BACK;
		stateW = Cmsg.buf & C_W_BACK ? -1 : 0;
		MsgSendPulse(displayWF, 10, TIMER_W_SET, 0);
		break;
	case 90://F11 S On/Off
		Amsg.buf ^= A_S;//������ ��������� �����
		break;
	case 65://F12 D On/Off
		Amsg.buf ^= A_D;//������ ��������� �����
		break;
	};
	if (c3 >= 86 && c3 <= 89)//���� ��� � ������� �
		MsgSend(coid_roby, &Cmsg, sizeof(Cmsg), NULL, NULL);
	else
		//���� ��� � ������� �
		MsgSend(coid_roby, &Amsg, sizeof(Amsg), NULL, NULL);
}
//������� ������ � ��������� ���������
void goStart() {
	Amsg.buf = 0;//�������� ������ ���������
	Cmsg.buf = 0;//��� ����������� ������ ��������
	Amsg.buf = A_X_BACK | A_Z_BACK | A_Y_BACK;//88 -��������� ����� ��� ����������
	MsgSend(coid_roby, &Amsg, sizeof(Amsg), NULL, NULL);//�������� �������
	control(87);//F<- ��������� � ������ �� F
	control(89);//W<- ��������� � ������ �� W
	Amsg.buf = 0;//�������� ������ ���������
	Cmsg.buf = 0;//��� ��� � ��������� ��������� ����� ����� �� �����
}
//����� ������ �� ����������. ��������� - ��� ���������� �������� ���������
void showSensors(unsigned char* _regC, unsigned char* _regB) {
	struct MESSAGE msg;//��������� ��� ���������
	msg.type = 2;//��� ���������� ����� �
	MsgSend(coid_roby, &msg, sizeof(msg), _regC, sizeof(unsigned char));
	msg.type = 3;//��� ���������� ����� �
	MsgSend(coid_roby, &msg, sizeof(msg), _regB, sizeof(unsigned char));
	printf("\n\r Sensors condition:");//����� �������� ��������-����������
	printf("\n\rXb: %c,", (*_regB) & B_X_BEGIN ? '1' : '0');
	printf(" Yb: %c,", (*_regB) & B_Y_BEGIN ? '1' : '0');
	printf(" Zb: %c,", (*_regB) & B_Z_BEGIN ? '1' : '0');
	printf(" Fb: %c,", (*_regC) & C_F_BEGIN ? '1' : '0');
	printf(" Fe: %c,", (*_regC) & C_F_END ? '1' : '0');
	printf(" Wb: %c,", (*_regB) & B_W_BEGIN ? '1' : '0');
	printf(" We: %c,", (*_regB) & B_W_END ? '1' : '0');
	printf(" S: %c,", Amsg.buf & A_S ? '1' : '0');//�� ���������, ��� ���
	printf(" D: %c", Amsg.buf & A_D ? '1' : '0');//��� �� �� ���
}
//����� � ������� ����
void showMenu() {
	printf("\nControl Roby \n");
	printf("\n    1st press    2nd press");
	printf("\nF1	X ->         X STOP ->");
	printf("\nF2	X <-         X STOP <-");
	printf("\nF3	Y ->         Y STOP ->");
	printf("\nF4	Y <-         Y STOP <-");
	printf("\nF5	Z ->         Z STOP ->");
	printf("\nF6	Z <-         Z STOP <-");
	printf("\nF7	F ->         F STOP ->");
	printf("\nF8	F <-         F STOP <-");
	printf("\nF9	W ->         W STOP ->");
	printf("\nF10	W <-         W STOP <-");
	printf("\nF11	S ON         S OFF    ");
	printf("\nF12	D ON         D OFF    ");
	printf("\nPress '+' for showing sensors data\n");
	printf("\nPress 'Enter' to go to start position\n\r");
}

int main() {
	unsigned char regC, regB;//���������� ��� �������� ��������� � � �
	//�������� ���������� � Roby
	const char coname[9] = "apu/roby";
	coid_roby = name_open(coname, 0);
	if (coid_roby == -1) {
		printf("\n Name not opened");
		return -1;
	}
	printf("\napu/roby has coid=%d", coid_roby);
	pid = getpid();//��������� Process Id
	//�������� �������� �������
	pthread_t displayXYZ_tid;
	pthread_t displayWF_tid;
	pthread_create(&displayXYZ_tid, NULL, &DisplayXYZ, NULL);
	sleep(2);
	pthread_create(&displayWF_tid, NULL, &DisplayWF, NULL);
	sleep(2);
	//���������� � ������� displayWF
	displayWF = ConnectAttach(0, pid, chidWF, 0, 0);
	sleep(2);
	showMenu();//����� ����
	Amsg.buf = 0;
	Amsg.type = 2;//������ �������� C
	MsgSend(coid_roby, &Amsg, sizeof(Amsg), &regC, sizeof(unsigned char));
	Amsg.type = 3;//������ �������� B
	MsgSend(coid_roby, &Amsg, sizeof(Amsg), &regB, sizeof(unsigned char));
	Amsg.type = 0;//�������� ��� ������ �������� �
	unsigned char startB = 0, startC = 0;
	startB = B_W_BEGIN | B_X_BEGIN | B_Y_BEGIN | B_Z_BEGIN;//0xF0
	startC = C_F_BEGIN;//0x08
	Cmsg.buf = 0;
	Cmsg.type = 1;
	x = regB & B_X_BEGIN ? 0 : x;//���� �� � ��������� ��� �������� ����������,
	y = regB & B_Y_BEGIN ? 0 : y;// �� ���������� ��� ���������
	z = regB & B_Z_BEGIN ? 0 : z;// ��� ��������������� ���������
	f_cnt = regC & C_F_BEGIN ? 0 : regC & C_F_END ? 1000 : f_cnt;
	w_cnt = regB & B_W_BEGIN ? 0 : regB & B_W_END ? 2000 : w_cnt;
	 if (regB != startB || regC != startC)//���� �� �� � ������
		goStart();//�� ������ ������ � ������
	int c1, c2, c3;
	raw(0);//������� ��������� � ����� ���������������� �����
	do {
		c1 = getchar();
		//printf("\r\nc1 = %d", c1);
		if (c1 == 27) {//ESC
			c2 = getchar();
			//printf("\r\nc2 = %d", c2);
			if (c2 == 79) {//F1-F12
				c3 = getchar();
				//printf("\r\nc3 = %d", c3);
				control(c3);
			} else if (c2 == 27)//ESC
				break;
		} else if (c1 == 43) { //'+'
			showSensors(&regC, &regB);
		} else if (c1 == 10) { //Enter
			goStart();
		}
	} while (1);
	unraw(0);
	printf("\nExit\n");
}
