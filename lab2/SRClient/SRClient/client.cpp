#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <time.h>

#pragma comment(lib,"ws2_32.lib")

#define SERVER_PORT 12340 //�������ݵĶ˿ں�
#define SERVER_IP "127.0.0.1" // �������� IP ��ַ
const int BUFFER_LENGTH = 1026;  //��������С������̫����UDP������֡�а�����ӦС��1480�ֽڣ�
const int RECEIVER_WIND_SIZE = 10;

const int SEQ_SIZE = 20;//���ն����кŸ�����Ϊ 1-20

BOOL hasSeq[SEQ_SIZE+1]; //seq���������falseΪ��Ӧ�յ���δ�յ�
int curSeq;


/****************************************************************/
/*  -time �ӷ������˻�ȡ��ǰʱ��
-quit �˳��ͻ���
-testsr [X] ���� SR Э��ʵ�ֿɿ����ݴ���
[X] [0,1] ģ�����ݰ���ʧ�ĸ���
[Y] [0,1] ģ�� ACK ��ʧ�ĸ���
*/
/****************************************************************/
void printTips() {
	printf("*****************************************\n");
	printf("|    -time to get current time |\n");
	printf("|    -quit to exit client |\n");
	printf("|    -testsr [X] [Y] to test the sr |\n");
	printf("*****************************************\n");
}


//************************************
// Method: lossInLossRatio
// FullName: lossInLossRatio
// Access: public
// Returns: BOOL
// Qualifier: ���ݶ�ʧ���������һ�����֣��ж��Ƿ�ʧ,��ʧ�򷵻�TRUE�����򷵻� FALSE
// Parameter: float lossRatio [0,1]
//************************************
BOOL lossInLossRatio(float lossRatio) {
	int lossBound = (int)(lossRatio * 100);
	int r = rand() % 101;
	if (r <= lossBound) {
		return TRUE;
	}
	return FALSE;
}


int main(int argc, char* argv[])
{
	printf("******************�ͻ���******************");
	//�����׽��ֿ⣨���룩
	WORD wVersionRequested;
	WSADATA wsaData;
	//�׽��ּ���ʱ������ʾ
	int err;
	//�汾 2.2
	wVersionRequested = MAKEWORD(2, 2);
	//���� dll �ļ� Scoket ��
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		//�Ҳ��� winsock.dll
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
	}
	else {
		printf("The Winsock 2.2 dll was found okay\n");
	}
	SOCKET socketClient = socket(AF_INET, SOCK_DGRAM, 0);  //�����ӡ����ɿ������ݱ�
	SOCKADDR_IN addrServer;
	addrServer.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);
	//���ջ�����
	char buffer[BUFFER_LENGTH];
	ZeroMemory(buffer, sizeof(buffer));
	char dataBuffer[1024 * 113];
	ZeroMemory(dataBuffer, 1024 * 113);
	int len = sizeof(SOCKADDR);
	//Ϊ�˲���������������ӣ�����ʹ�� -time ����ӷ������˻�õ�ǰʱ��
	//ʹ�� -testsr [X] [Y] ���� GBN ����[X]��ʾ���ݰ���ʧ����
	//					[Y]��ʾ ACK ��������
	printTips();
	int ret;
	int interval = 1;//�յ����ݰ�֮�󷵻�ACK�ļ����Ĭ��Ϊ 1 ��ʾÿ��������ACK�� 0 ���߸�������ʾ���еĶ�������ACK
	char cmd[128];
	float packetLossRatio = 0.1; //Ĭ�ϰ���ʧ�� 0.1
	float ackLossRatio = 0.1;   //Ĭ��ACK��ʧ�� 0.1
								//��ʱ����Ϊ������ӣ�����ѭ����������
	srand((unsigned)time(NULL));
	while (true) {
		gets_s(buffer);
		ret = sscanf(buffer, "%s%f%f", &cmd, &packetLossRatio, &ackLossRatio);
		//��ʼ SR ���ԣ�ʹ�� SR Э��ʵ�� UDP �ɿ��ļ�����
		if (!strcmp(cmd, "-testsr")) {
			printf("%s\n", "Begin to test SR protocol, please don't abort the process");
			printf("The loss ratio of packet is %.2f,the loss ratio of ack is %.2f\n", packetLossRatio, ackLossRatio);
			int stage = 0;
			BOOL b;
			unsigned char u_code;  //״̬��
			unsigned short seq;  //�������к�
			unsigned short recvSeq;  //���մ��ڴ�СΪ 10����ȷ�ϵ����к�
			unsigned short waitSeq;  //�ȴ������к�
			hasSeq[0] = TRUE;   //���к�Ϊ1-20
			sendto(socketClient, "-testsr", strlen("-testsr") + 1, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
			while (true)
			{
				//�ȴ� server �ظ����� UDP Ϊ����ģʽ
				recvfrom(socketClient, buffer, BUFFER_LENGTH, 0, (SOCKADDR*)&addrServer, &len);
				switch (stage) {
				case 0:  //�ȴ����ֽ׶�
					u_code = (unsigned char)buffer[0];
					if ((unsigned char)buffer[0] == 205)
					{
						printf("Ready for file transmission\n");
						buffer[0] = 200;
						buffer[1] = '\0';
						sendto(socketClient, buffer, 2, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
						stage = 1;
						recvSeq = 0;
						waitSeq = 1;
					}
					break;
				case 1:  //�ȴ��������ݽ׶�
					seq = (unsigned short)buffer[0];
					//�����ģ����Ƿ�ʧ
					b = lossInLossRatio(packetLossRatio);
					if (b) {
						printf("The packet with a seq of %d loss\n", seq);
						continue;
					}
					printf("recv a packet with a seq of %d, \trecvSeq:%d\n", seq, recvSeq);

					//������ڴ��İ�����ȷ���գ�����ȷ�ϼ���
					int step = seq - recvSeq;
					step = step >= 0 ? step : step + SEQ_SIZE;
					//����ڽ��մ�����
					if (step < RECEIVER_WIND_SIZE) {
						hasSeq[seq] = TRUE;
						BOOL flag = TRUE; //�жϴ�recvSeq��seq�Ƿ񶼱�����
						if (recvSeq <= seq) {
							for (int i = recvSeq; i <= seq; i++) {
								if (!hasSeq[i]) {
									flag = FALSE;
								}
							}
						}
						else {
							for (int i = recvSeq; i <= SEQ_SIZE; i++) {
								if (!hasSeq[i]) {
									flag = FALSE;
								}
							}
							for (int i = 1; i <= seq; i++) {
								if (!hasSeq[i]) {
									flag = FALSE;
								}
							}
						}
						if (flag) {
							recvSeq = seq;
						}
						//�������
						printf("%s\n", &buffer[1]);
						buffer[0] = seq;
						buffer[1] = '\0';
					}
					else if (recvSeq > seq && (recvSeq - seq) <= RECEIVER_WIND_SIZE) {
						buffer[0] = seq;
						buffer[1] = '\0';
					}
					else {
						//�����ǰһ������û���յ�����ȴ� Seq Ϊ 1 �����ݰ��������򲻷��� ACK����Ϊ��û����һ����ȷ�� ACK��
						if (!recvSeq) {
							continue;
						}
						buffer[0] = recvSeq;
						buffer[1] = '\0';
					}
					b = lossInLossRatio(ackLossRatio);
					if (b) {
						printf("The ack of %d loss\n", (unsigned char)buffer[0]);
						continue;
					}
					sendto(socketClient, buffer, 2, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
					printf("send a ack of %d\n", (unsigned char)buffer[0]);
					break;
				}
				Sleep(500);
			}
		}
		sendto(socketClient, buffer, strlen(buffer) + 1, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
		ret = recvfrom(socketClient, buffer, BUFFER_LENGTH, 0, (SOCKADDR*)&addrServer, &len);
		printf("%s\n", buffer);
		if (!strcmp(buffer, "Good bye!")) {
			break;
		}
		printTips();
	}
	//�ر��׽���
	closesocket(socketClient);
	WSACleanup();
	return 0;
}