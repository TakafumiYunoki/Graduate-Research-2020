#include <stdio.h>        /*printf()*/
#include <stdlib.h>       /*exit()*/
#include <string.h>       /*strlen()*/
#include <unistd.h>       /*close()*/
#include <sys/socket.h>   /*socket(),connect()*/
#include <sys/types.h>    /*setsockopt()*/
#include <arpa/inet.h>    /*struct sockaddr_in*/
#include <netinet/in.h>   /*inet_addr()*/
#include <fcntl.h>        /*open()*/
#include <time.h>         /*clock_t,time_t*/
#include <sys/time.h>     /*struct timeval*/

#define DEV_RANDOM "/dev/urandom"  //乱数生成用のデバイスファイル名
#define AUTHDATA_BYTE 4            //認証情報のビット数
#define DATAMAX 4294967295         //32bitの上限値

static const unsigned int crc32tab[256] = { 
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
  0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
  0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
  0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
  0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
  0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
  0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
  0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
  0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
  0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
  0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
  0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
  0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
  0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
  0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
  0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
  0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
  0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

//指定されたビット数の範囲内で乱数値を出力する関数
unsigned long get_random(unsigned char *const buf, const int bufLen, const int len){
  //初期エラー
  if(len > bufLen){
    perror("buffer size is small\n");
    return -1;
  }
  
  int fd = open(DEV_RANDOM, O_RDONLY);
  unsigned long result=0b0;
  register int i;
  if(fd == -1){
    printf("can not open %s\n", DEV_RANDOM);
    return -1;
  }
  
  int r = read(fd,buf,len);
  if(r < 0){
    perror("can not read\n");
    return -1;
  }
  if(r != len){
    perror("can not read\n");
    return -1;
  }
  close(fd);
  
  for(i=0; i<AUTHDATA_BYTE; i++){
    result=(result<<8)+buf[i];
  }
  return result;
}

//crc32のハッシュ値を出力する関数
unsigned long crc32(const unsigned char *s,int len){
  unsigned int crc_init = 0;
  unsigned int crc =0;

  crc = crc_init ^ 0xFFFFFFFF;
  for(;len--;s++){
    crc = ((crc >> 8)&0x00FFFFFF) ^ crc32tab[(crc ^ (*s)) & 0xFF];
  }
  return crc ^ 0xFFFFFFFF;
}

double getETime(){
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + (double)tv.tv_usec*1e-6;
}

int main(int argc, char **argv){

  //ソケット通信用
  register int sock;
  struct sockaddr_in server_addr;
  unsigned short servPort;
  char *servIP;

  //通信データ用バッファサイズ
  int rcvBufferSize=100*1024;
  int sendBufferSize=100*1024;

  //データ長
  unsigned short length;

  //各種フラグ
  unsigned short authent_flag=0;
  unsigned short overflow_flag=1;

  FILE *fp;

  //ループ用変数
  register int i;
  register int n;

  //認証情報用
  unsigned char id[34]={'\0'};
  unsigned char pass[AUTHDATA_BYTE+2]={'\0'};
  unsigned long pass_num,A,pre_A,A2,pre_A2,B,F,alpha,beta,gamma,Ni,Nii;

  //各関数用バッファ
  unsigned char hash_buf[20]={'\0'};
  unsigned char rand_buf[AUTHDATA_BYTE]={'\0'};

  //暗号通信用
  unsigned char vernamKey[AUTHDATA_BYTE]={'\0'};
  unsigned char vernamData[AUTHDATA_BYTE+1]={'\0'};

  /*時間計測*/
  clock_t start,end;
  double st,en;
  int calc_times;
  register int z;

  //実行時の入力エラー
  if((argc<3) || (argc>4)){
    fprintf(stderr,"Usage: %s <Server IP> <Server Port> <calc times>\n",argv[0]);
    exit(1);
  }
  
  servIP = argv[1]; //server IP設定
  //ポート番号設定
  if(argc==4){
    servPort=atoi(argv[2]);    //ポート番号設定
    calc_times=atoi(argv[3]);  //測定回数設定
  }
  else{
    servPort=7777;
    calc_times=atoi(argv[2]);
  }
    
  //サーバ接続用ソケットを作成
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("client: socket\n");
    exit(1);
  }
  
  //サーバアドレス用構造体の初期設定
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = PF_INET;
  server_addr.sin_addr.s_addr = inet_addr(servIP);
  server_addr.sin_port = htons(servPort);

  
  //サーバと接続
  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("client: connect\n");
    exit(1);
  }

  //バッファサイズを設定
  if(setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&rcvBufferSize,sizeof(rcvBufferSize))<0){
    perror("setsockopt() failed\n");
    exit(1);
  }
  if(setsockopt(sock,SOL_SOCKET,SO_SNDBUF,&sendBufferSize,sizeof(sendBufferSize))<0){
    perror("setsockopt() failed\n");
    exit(1);
  }
  
  //フラグを受け取る
  read(sock, &authent_flag, sizeof(unsigned short));
  printf("flag=%d\n",authent_flag);
  
  if(authent_flag==0){
    
    //初回登録フェーズ
    printf("このユーザは未登録のため、初回登録を行います\n\n");

    //Niを作成
    Ni=get_random(rand_buf,AUTHDATA_BYTE,AUTHDATA_BYTE);
    /////////////////////////////
    //
    printf("Ni->%lu\n",Ni);
    //
    /////////////////////////////
    
    //Niを保存
    if((fp=fopen("./../client_data/Ni_file.bin","wb"))==NULL){
      perror("Ni FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fwrite(&Ni,sizeof(unsigned long),1,fp);
    fclose(fp);
    
    printf("ID:");
    //最大32文字の入力を受け入れる
    if (fgets(id, 34, stdin)==NULL || id[0]=='\0' || id[0] == '\n'){
      perror("id:1-32char\n");
      close(sock);
      exit(1);
    }
    for(i=1; i<34; i++){
      if(id[i]=='\n'){
	overflow_flag=0;
      }
    }
    if(overflow_flag==1){
      perror("id:1-32char\n");
      close(sock);
      exit(1); 
    }
    overflow_flag=1;
    
    //改行を削除
    length=strlen(id);
    if(id[length-1]=='\n'){
      id[--length]='\0';
    }

    //送信するidの長さの情報を送信する
    write(sock, &length, sizeof(unsigned short));

    //IDを送信する
    write(sock, id, length);
    
    printf("PASS:");
    //最大4文字の入力を受け入れる
    if (fgets(pass, AUTHDATA_BYTE+2, stdin)==NULL || pass[0]=='\0' || pass[0]=='\n'){
      perror("PASS:1-4char\n");
      close(sock);
      exit(1);
    }
    for(i=1; i<AUTHDATA_BYTE+2; i++){
      if(pass[i]=='\n'){
	overflow_flag=0;
      }
    }
    if(overflow_flag==1){
      perror("pass:1-4char\n");
      close(sock);
      exit(1); 
    }
    overflow_flag=1;

    //改行を削除
    length=strlen(pass);
    if(pass[length-1]=='\n'){
      pass[--length]='\0';
    }

    //パスワードを数値に変換
    pass_num=0;
    for(i=0; i<AUTHDATA_BYTE; i++){
      pass_num=(pass_num<<8)+pass[i];
    }

    //Aを作成
    pre_A=pass_num^Ni;
    sprintf(hash_buf,"%lu",pre_A);
    A=crc32(hash_buf,strlen(hash_buf));

    //////////////////////////////
    //デバッグ用
    printf("Ni^S->%lu\n",pre_A);
    printf("A->%lu\n",A);
    //
    //////////////////////////////////

    //Aを送信する
    write(sock, &A, sizeof(unsigned long));

    close(sock);
    exit(0);
  }
  else if(authent_flag==1){
    printf("このクライアントは登録済みです\n\n");
  }
  else{
   perror("flag error!\n");
   close(sock);
   exit(1);
  }

  //認証フェーズ
  printf("PASS:");
  if (fgets(pass, AUTHDATA_BYTE+2, stdin)==NULL || pass[0]=='\0' || pass[0]=='\n'){
    perror("PASS:1-4char\n");
    close(sock);
    exit(1);
  }
  for(i=1; i<AUTHDATA_BYTE+2; i++){
    if(pass[i]=='\n'){
      overflow_flag=0;
    }
  }
  if(overflow_flag==1){
    perror("pass:1-4char\n");
    close(sock);
    exit(1); 
  }
  overflow_flag=1;
  
  //改行を削除
  length=strlen(pass);
  if(pass[length-1]=='\n'){
    pass[--length]='\0';
  }

  //パスワードを数値に変換
  pass_num=0;
  for(i=0; i<AUTHDATA_BYTE; i++){
    pass_num=(pass_num<<8)+pass[i];
  }

  /*計測開始*/
  puts("計測開始:");
  st = getETime();
  start = clock();
  for(z=calc_times; z>0; z--){
    
    if((fp=fopen("./../client_data/Ni_file.bin","rb"))==NULL){
      perror("Ni FILE OPEN ERROR\n");
      close(sock);
      exit(1);
    }
    fread(&Ni,sizeof(unsigned long),1,fp);
    fclose(fp);
  
    //Aの作成
    pre_A=pass_num^Ni;
    sprintf(hash_buf,"%lu",pre_A);
    A=crc32(hash_buf,strlen(hash_buf));
  
    //////////////////////////////
    //デバッグ用
    //printf("Ni^S->%lu\n",pre_A);
    //printf("A->%lu\n",A);
    //
    //////////////////////////////////
  
    //Ni+1を作成
    Nii=get_random(rand_buf,AUTHDATA_BYTE,AUTHDATA_BYTE);
    ///////////////////////////////
    //
    //printf("Ni+1->%lu\n",Nii);
    //
    //////////////////////////////
  
    //Ai+1を作成
    pre_A2=pass_num^Nii;
    sprintf(hash_buf,"%lu",pre_A2);
    A2=crc32(hash_buf,strlen(hash_buf));
  
    /////////////////////////////////
    //
    //printf("Ni+1^S->%lu\n",pre_A2);
    //printf("Ai+1->%lu\n",A2);
    //
    /////////////////////////////////

    //B=H(Ai+1)を作成
    sprintf(hash_buf,"%lu",A2);
    B=crc32(hash_buf,strlen(hash_buf));
  
    ////////////////////////////////
    //
    //printf("B->%lu\n",B);
    //
    ////////////////////////////////
  
    //alphaを作成
    if(B+A>DATAMAX){
      alpha=A2^(B+A-DATAMAX);
    }
    else{
      alpha=A2^(B+A);
    }
    ///////////////////////////////
    //
    //printf("alpha->%lu\n",alpha);
    //
    //////////////////////////////
  
    //betaを作成
    beta=B^A;
    /////////////////////////////
    //
    //printf("beta->%lu\n",beta);
    //
    ////////////////////////////
  
    //alphaを送信する
    write(sock, &alpha, sizeof(unsigned long));
  
    //betaを送信する
    write(sock, &beta, sizeof(unsigned long));
  
    //gammaを受け取る
    read(sock, &gamma, sizeof(unsigned long));
    ////////////////////////////
    //
    //printf("gamma->%lu\n",gamma);
    //
    ////////////////////////////
  
    //F=H(B)を作成する
    sprintf(hash_buf,"%lu",B);
    F=crc32(hash_buf,strlen(hash_buf));
  
    ////////////////////////////////
    //
    //printf("F->%lu\n",F);
    //
    ////////////////////////////////
  
    if(gamma-F != 0){
      perror("gamma and F are not equal\n");
      perror("authentication error!\n");
      close(sock);
      exit(1);
    }
    else{
      //printf("gamma and F equal\n");
    
      //次の認証情報をファイルに保存
      if((fp=fopen("./../client_data/Ni_file.bin","wb"))==NULL){
	perror("Ni FILE OPEN ERROR\n");
	close(sock);
	exit(1);
      }
      fwrite(&Nii,sizeof(unsigned long),1,fp);
      fclose(fp);
    }
  }
  /*計測終了*/
  end = clock();
  en = getETime();
      
  //暗号通信フェーズ
  puts("-----------------------");

  for(i=0; i<AUTHDATA_BYTE; i++){
    vernamKey[i]=(A2 & ((unsigned long)0xFF<<24-8*i))>>24-8*i;
  }
  
  ///////////////////////////////////
  //
  printf("vernam_key:");
  for(i=0; i<AUTHDATA_BYTE; i++){
    printf("%02x",vernamKey[i]);
  }
  printf("\n");
  //
  //////////////////////////////////
  
  
  //バーナム暗号用メッセージを読み取り表示
  if((fp=fopen("./../client_data/msg.txt","r"))==NULL){
    perror("msg_file open error!\n");
    close(sock);
    exit(1);
  }
  fgets(vernamData,AUTHDATA_BYTE+1,fp);

  /////////////////////////////////
  //
  printf("平文(上限4文字)：%s\n",vernamData);
  //
  //////////////////////////////////
  
  //暗号化
  i=0;
  length=strlen(vernamData);
  while(i<length){
    vernamData[i]^=vernamKey[i];
    i++;
  }
  vernamData[i]='\0';
  
  ////////////////////////////////////
  //
  printf("暗号文(16進数):");
  for(i=0; i<length; i++){
    printf("%02x",vernamData[i]);
  }
  printf("\n");
  //
  ///////////////////////////////////
  
  //送信するmsgの長さの情報を送信する
  write(sock, &length, sizeof(unsigned short));
  
  //msgを送信する
  write(sock, vernamData,length);
  close(sock);

  //計測結果出力
  printf("CPU時間 :%.6f s\n",(double)(end-start)/(CLOCKS_PER_SEC));
  printf("経過時間:%.6f s\n",en-st);

  //測定用
  puts("-----------------------");
  printf("process number -> %d\nPlease input Ctrl+C...\n",getpid());
  while(1){
  }
  
  exit(0);
}
