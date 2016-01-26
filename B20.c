extern  float end_read_b20(void);   // 读取DS18B20温度传感器的数据
extern unsigned int write_b20(unsigned int data); //写B20数据  
extern void delay(int data);          //延迟
extern void init_B20(void);           //初始化B20
extern int read_bit_B20(void);        //读取1bit,逻辑上必须成一函数
extern int read_byte_B20(void);       //读取一个完整字节
extern void start_shift_B20(void);    //启用B20开始温度转化，最低750ms
extern float read_wendu_B20(void);    //读温度
extern void read_rom_B20(void);       //每个B20都有唯一ROM


#define B20_OUT  {1 << 3, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}  //读取温度传感器应该输入输出双向配置
#define B20_IN   {1 << 3, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PIN_wendu_out B20_OUT
#define PIN_wendu_in  B20_IN

static const Pin pinsSPI[] = {PIN_SPI};
static const Pin pin_wendu_out[] = {PIN_wendu_out};
static const Pin pin_wendu_in[] = {PIN_wendu_in};

#define DQ_OUT  &pin_wendu_out[0] //引脚输出
#define DQ_IN  &pin_wendu_in[0]  //引脚输入

/*
初始化时序步骤：  
1. 主机将端口设为输出，先发送一个高电平，然后再拉低，维持480-960US；（推荐500-600US）
2. 主机将端口设为输入，上拉电阻此时将电平拉高，主机等待60US-200US；（推荐100-150US） 
3. 主机读取端口数据，低电平则初始化成功；高电平表示初始化失败； 
4.  读取数据完毕后，主机等待至少400US；  （
     推荐450-500US） 注：第四步很重要，读取初始化状态后，仍然延时400US才可以初始化完毕，否
      则传感器不能 正常使用；  ? 
      在这里注意端口需要不停地改变方向；
      在主机发送时，设为输出，主机接收时，设为输出； 
*/
//----------------------------------------------------------------------------        
//延迟函数，48MHZ主频，延迟120个数=20us
//----------------------------------------------------------------------------
void delay(int data){
   int count=data;
   while(count !=0){count--;}
 }
//----------------------------------------------------------------------------        
//初始化B20
//----------------------------------------------------------------------------
void init_B20(void){
    int c;   
    PIO_Configure( pin_wendu_out, PIO_LISTSIZE( pin_wendu_out));
    delay(12); //延迟2微秒 
    PIO_Clear(DQ_OUT);
    delay(120*16); //延迟600微秒
    PIO_Configure(pin_wendu_in, PIO_LISTSIZE( pin_wendu_in));
    delay(120*3); //延迟60微秒    
    c=PIO_Get(DQ_IN );    
    delay(120*26); //延迟20*26微秒 
   // return c;
}
//----------------------------------------------------------------------------        
//读取1bit,逻辑上必须成一函数，方便按位频繁读写
//----------------------------------------------------------------------------
int read_bit_B20(void){
      int c;
      PIO_Configure( pin_wendu_out, PIO_LISTSIZE( pin_wendu_out));
      PIO_Clear(DQ_OUT);
      asm("nop");
      PIO_Configure(pin_wendu_in, PIO_LISTSIZE( pin_wendu_in));
      delay(48); //9us
      c=PIO_Get(DQ_IN );
      delay(216); //36us
      return  c;
 
 }
//----------------------------------------------------------------------------        
//读取一个完整字节，包含温度数据以及其它数据
//----------------------------------------------------------------------------
int  read_byte_B20(void){
   int i,j=0;
   char temp;
   char temp2;
   int temp3;
   char c;  
   for(i=0;i<8;i++){    
       j=read_bit_B20();
       temp = (j<<7) |(temp>>1);    
       delay(216); //36us          
   }
   return temp;
}
//----------------------------------------------------------------------------        
//启用B20开始温度转化，最低750ms的温度转化,
//----------------------------------------------------------------------------
void start_shift_B20(void){
      int m,n;
      init_B20();
      delay(54);//延时9us
      write_b20(0xCC);
      delay(54);//延时9us
      // write_b20(0x64);
      write_b20(0x44);
      for(n=0;n<=600;n++){
        delay(120*38); //延时760ms
      }        
      for(n=0;n<=1000;n++){
        delay(120*38);  //延时760ms
     }
}
//----------------------------------------------------------------------------        
////读温度
//----------------------------------------------------------------------------
float read_wendu_B20(void){
   // int wdL,wdH,
    int temp;
    float temp_wendu;
     init_B20();
	  write_b20(0xCC);  //忽略ROM指令
	  write_b20(0xBE);   //读暂存器指令，
	   wdL=read_byte_B20();
	   wdH=read_byte_B20();
           //  wdL=read_byte_B20();
	  // wdH=read_byte_B20();
           //  wdL=read_byte_B20();
	  // wdH=read_byte_B20();
	   temp=wdH;
	   temp=temp<<8;
	   temp=temp|wdL;	  
	   temp_wendu=((float)temp*0.0625);
         return  temp_wendu;
}
//----------------------------------------------------------------------------        
//每个B20都有唯一ROM
//----------------------------------------------------------------------------
void read_rom_B20(void){
     int j ;
     init_B20();
     write_b20(0x33);
    for(j=0;j<=7;j++){     //读64bit ROM
      sn[j]=read_byte_B20();
     asm("nop");
     }
}
//---------------------------------------------------------------------------         
// 温度传感器读温度函数
//----------------------------------------------------------------------------
float end_read_b20(void){
  int temp;
   PIO_Configure( pin_wendu_out, PIO_LISTSIZE( pin_wendu_out));
        init_B20();
	read_rom_B20();
        while(1){
        start_shift_B20();
        temp= read_wendu_B20();
         USART_WriteBuffer(AT91C_BASE_US0,&wdL,1);
         USART_WriteBuffer(AT91C_BASE_US0,&wdH,1);
        }
	return temp;
}

/*-------------------------------------
//主函数入口，测试例子
-------------------------------------*/
int main(void)
{     
 char tan_pin[100];
 start_shift_B20();                   //启用转换
 temp_tan= read_wendu_B20();          //读温度
 sprintf(tan_pin, "%.3f",temp_tan );  //转化成字符串，便于传输显示，保留3位精度
         /*
            //用户自己添加事件event
          */
}
