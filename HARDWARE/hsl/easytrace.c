#include "easytrace.h"
#include "LCD.h"
#include "sys.h"
#include "Py.h"
#include "delay.h"
#include "usart.h"
#define min3v(v1, v2, v3)   ((v1)>(v2)? ((v2)>(v3)?(v3):(v2)):((v1)>(v3)?(v3):(v1)))//取最大
#define max3v(v1, v2, v3)   ((v1)<(v2)? ((v2)<(v3)?(v3):(v2)):((v1)<(v3)?(v3):(v1)))//取最小值

int failed_center=6;
float p=0.5;
int fail=10;
extern int shibie_r_or_g[6];
extern int red;
typedef struct
	  {
    unsigned char  red;             // [0,255]
    unsigned char  green;           // [0,255]
    unsigned char  blue;            // [0,255]
    }COLOR_RGB;//RGB格式颜色

typedef struct
	  {
    unsigned char hue;              // [0,240]
    unsigned char saturation;       // [0,240]
    unsigned char luminance;        // [0,240]
    }COLOR_HSL;//HSL格式颜色

typedef struct
	  {
    unsigned int X_Start;              
    unsigned int X_End;
	  unsigned int Y_Start;              
    unsigned int Y_End;
    }SEARCH_AREA;//区域

		int x_z=0,y_z=0,w_z=0,h_z=0,xy=0;
		int zhen=0;
		int mum=0;
//读取RBG格式颜色，唯一需要移植的函数
extern unsigned short LCD_ReadPoint(unsigned short x,unsigned short y);//读某点颜色

static void ReadColor(unsigned int x,unsigned int y,COLOR_RGB *Rgb)

	{
		unsigned short C16;

		C16 = LCD_ReadPoint(x,y);     //读某点颜色

		Rgb->red   =	 (unsigned char)((C16&0xf800)>>8);
		Rgb->green =	 (unsigned char)((C16&0x07e0)>>3);
		Rgb->blue  =   (unsigned char)((C16&0x001f)<<3);
	}



//RGB转HSL
static void RGBtoHSL(const COLOR_RGB *Rgb, COLOR_HSL *Hsl)
	{
			int h,s,l,maxVal,minVal,difVal;
			int r  = Rgb->red;
			int g  = Rgb->green;
			int b  = Rgb->blue;
		
				maxVal = max3v(r, g, b);
				minVal = min3v(r, g, b);
				
				difVal = maxVal-minVal;
		
		//计算亮度
			l = (maxVal+minVal)*240/255/2;
		
		if(maxVal == minVal)//若r=g=b
			{
				h = 0; 
				s = 0;
			}
		else
		{
			//计算色调
			if(maxVal==r)
			{
				if(g>=b)
					h = 40*(g-b)/(difVal);
				else
					h = 40*(g-b)/(difVal) + 240;
			}
			else if(maxVal==g)
					h = 40*(b-r)/(difVal) + 80;
			else if(maxVal==b)
					h = 40*(r-g)/(difVal) + 160;
			//计算饱和度
			if(l == 0)
					s = 0;
			else if(l<=120)
					s = (difVal)*240/(maxVal+minVal);
			else
					s = (difVal)*240/(511 - (maxVal+minVal));
		}
			Hsl->hue =        (unsigned char)(((h>240)? 240 : ((h<0)?0:h)));//色度
			Hsl->saturation = (unsigned char)(((s>240)? 240 : ((s<0)?0:s)));//饱和度
			Hsl->luminance =  (unsigned char)(((l>240)? 240 : ((l<0)?0:l)));//亮度
		 // printf ("(r=%d,g=%d,b=%d)\r\n",r,g,b);
//		  printf ("(h=%d,s=%d,l=%d)\r\n",h,s,l);
	}

//匹配颜色
static int ColorMatch(const COLOR_HSL *Hsl,const TARGET_CONDI *Condition)//HSL格式颜色、判定为的目标条件
	{
	int t=6;
				if(
							( 
									(Hsl->hue		>=	Condition->H_MIN) &&
									(Hsl->hue		<=	Condition->H_MAX) &&
									(Hsl->saturation	>=	Condition->S_MIN) &&
								(Hsl->saturation	<=   Condition->S_MAX )&&
									(Hsl->luminance	>=	Condition->L_MIN) &&
									(Hsl->luminance	<=   Condition->L_MAX) 
							)  
				|| 
						(   
									( red ) &&(Hsl->hue		>=	(unsigned char)shibie_r_or_g[0]) &&
									(Hsl->hue		<=	(unsigned char)shibie_r_or_g[1]) &&
									(Hsl->saturation	>=	(unsigned char)shibie_r_or_g[2]) &&
								(Hsl->saturation	<=  (unsigned char)shibie_r_or_g[3])&&
									(Hsl->luminance	>=	(unsigned char)shibie_r_or_g[4]) &&
									(Hsl->luminance	<=  (unsigned char) shibie_r_or_g[5]) 
					 )		
						 
									
				)          //hue为色调，saturation为饱和度 ，luminance为亮度
							
						
		 
		 return 1;
								
		else
				return 0;
	
	
	
	}

//搜索腐蚀中心
static int SearchCentre(unsigned int *x,unsigned int *y,const TARGET_CONDI *Condition,const SEARCH_AREA *Area)
//TARGET_CONDI判定为的目标条件、SEARCH_AREA区域
	{
			unsigned int SpaceX,SpaceY,i,j,k,FailCount=0;
			COLOR_RGB Rgb;
			COLOR_HSL Hsl;
			
//			SpaceX = Condition->WIDTH_MIN/3;   //目标最小宽度
//			SpaceY = Condition->HIGHT_MIN/3;   //目标最小高度
        SpaceX =20;   //目标最小宽度
			  SpaceY =20;   //目标最小高度
				for(i=Area->Y_Start;i<Area->Y_End;i+=SpaceY)
				{
					for(j=Area->X_Start;j<Area->X_End;j+=SpaceX)
					{
						FailCount=0;
						for(k=0;k<SpaceX+SpaceY;k++)
						{
							if(k<SpaceX)
								ReadColor(j+k,i+SpaceY/2,&Rgb);
							else
								ReadColor(j+SpaceX/2,i+(k-SpaceX),&Rgb);
							
							  RGBtoHSL(&Rgb,&Hsl);
							
							
							if(!ColorMatch(&Hsl,Condition))
								FailCount++;
						
						if(FailCount>failed_center)
								break;
						}
						if(k==SpaceX+SpaceY)
						{
							*x = j+SpaceX/2;
							*y = i+SpaceY/2;
							return 1;
						}
					}
				}
		return 0;
	}

//从腐蚀中心向外腐蚀，得到新的腐蚀中心
static int Corrode(unsigned int oldx,unsigned int oldy,const TARGET_CONDI *Condition,RESULT *Resu)
{
	unsigned int Xmin,Xmax,Ymin,Ymax,i,FailCount=0;
	
	unsigned int Xmin1,Xmax1,Ymin1,Ymax1;
	
	COLOR_RGB Rgb;
	COLOR_HSL Hsl;   
	
	 int n=0,x1=0,x2=0,y1=0,y2=0;
	float t=0;
	
	
	int chi=10;
	
	
	for(i=oldx;i>IMG_X;i--)
		{  		  
				ReadColor(i,oldy,&Rgb);
				RGBtoHSL(&Rgb,&Hsl);
				if(!ColorMatch(&Hsl,Condition))
					FailCount++;
				else
				{
					n++;
					x1=x1+i;
				}
				if(n>chi)
				{
				t=n/(float)(n+FailCount);
				}
				if((FailCount>fail)&&(t>p))
					{
//					if(FailCount>fail)printf("FailCount33=%d   \r\n",FailCount);
//					if(t>p)printf("t33=%f  \r\n",t);	
//						printf(" n=%d   \r\n",n);
					break;	
				
				}
		}
	Xmin=i;
//		printf("Count=%d   \r\n",(n+FailCount));printf("t=%f  \r\n",t);	printf("FailCount=%d   \r\n",FailCount);
//		printf("p1=%f   \r\n",t);	
	Xmin1=x1/n;
		
	FailCount=0;
	t=0;
	n=0;
	for(i=oldx;i<IMG_X+IMG_W;i++)
		{
				ReadColor(i,oldy,&Rgb);
				RGBtoHSL(&Rgb,&Hsl);
				if(!ColorMatch(&Hsl,Condition))
					FailCount++;
					else
				{
					n++;	
					x2+=i;
				}
				if(n>chi){
				t=n/(float)(n+FailCount);
				}
			if((FailCount>fail)&&(t>p))
					break;	
		}
		
	Xmax1=x2/n;
//		printf("p22=%f   \r\n",t);	
  Xmax=i;
	FailCount=0;
	t=0;
	n=0;
	
	for(i=oldy;i>IMG_Y;i--)
		{
				ReadColor(oldx,i,&Rgb);
				RGBtoHSL(&Rgb,&Hsl);
				if(!ColorMatch(&Hsl,Condition))
					FailCount++;
					else
				{
					n++;	
					y1+=i;
				}
			if(n>chi){
				t=n/(float)(n+FailCount);
				}
		if((FailCount>fail)&&(t>p))
					break;	
		}
	Ymin=i;
//		printf("p33=%f   \r\n",t);	
		Ymin1=y1/n;
		
	FailCount=0;
	t=0;
	n=0;
	
	for(i=oldy;i<IMG_Y+IMG_H;i++)
		{
				ReadColor(oldx,i,&Rgb);
				RGBtoHSL(&Rgb,&Hsl);
				if(!ColorMatch(&Hsl,Condition))
					FailCount++;
					else
				{
					n++;	
					y2+=i;
				}
			if(n>chi){
				t=n/(float)(n+FailCount);
				}
//					if((FailCount>fail)&&(t<p))
				if((FailCount>fail)&&(t>p))
					break;	
		}
	Ymax1=y2/n;
	Ymax=i;
//		printf("p44=%f   \r\n",t);	
	FailCount=0;
	t=0;
	n=0;
	
	Resu->x	= (Xmin1+Xmax1)/2;
	Resu->y	= (Ymin1+Ymax1)/2;
	Resu->w	= Xmax-Xmin-fail;
	Resu->h	= Ymax-Ymin-fail;
   if(xy==1)
		{
		  x_z += Resu->x;
	   	y_z+=Resu->y;
			w_z+=Resu->w;
			h_z  = Resu->h;
			mum++;
		}
		
		
	if(
		    	((Resu->x-Resu->w/2)>0)&&((Resu->x-Resu->w/2)<240)&&((Resu->y-Resu->h/2)>0)&&((Resu->y-Resu->h/2)<320)&&
			 ((Xmax-Xmin)>(Condition->WIDTH_MIN)) && ((Ymax-Ymin)>(Condition->HIGHT_MIN)) &&
			 ((Xmax-Xmin)<(Condition->WIDTH_MAX)) && ((Ymax-Ymin)<(Condition->HIGHT_MAX))
	   )
		  return 1;	
	else
		  return 0;	
}

//唯一的API，用户将识别条件写入Condition指向的结构体中，该函数将返回目标的x，y坐标和长宽
//返回1识别成功，返回1识别失败
int Trace(const TARGET_CONDI *Condition,RESULT *Resu)  //TARGET_CONDI识别条件  RESULT识别结果
{
	unsigned int i;
	static unsigned int x0,y0,flag=0;  //静态变量
	static SEARCH_AREA Area={IMG_X,IMG_X+IMG_W,IMG_Y,IMG_Y+IMG_H};  //x坐标 y坐标  w宽度  h高度
	RESULT Result;	//RESULT识别结果
	

	if(flag==0)    //已经定义
		{
			if(SearchCentre(&x0,&y0,Condition,&Area))  //搜索腐蚀中心   &是取地址运算符
				 flag=1;
			else
				{
						Area.X_Start= IMG_X	       ;
						Area.X_End  = IMG_X+IMG_W  ;
						Area.Y_Start= IMG_Y		     ;
						Area.Y_End  = IMG_Y+IMG_H  ;

						if(!SearchCentre(&x0,&y0,Condition,&Area))	
						{
							flag=0;
							return 0;
						}	
				}
		}
	Result.x = x0;
	Result.y = y0;
	
	for(i=0;i<ITERATE_NUM;i++) 		//进行迭代计算
		{
		   zhen=1;
			if(i>ITERATE_NUM/2) xy=1;
			Corrode(Result.x,Result.y,Condition,&Result);	
		}
		
		
		
		Result.x=x_z/mum;
			Result.y=y_z/mum;
			Resu->w=w_z/mum;
			Resu->h=h_z/mum;
	mum=0;
	xy=0;
	x_z=0;
	y_z=0;
	zhen=0;
	if(Corrode(Result.x,Result.y,Condition,&Result))//从腐蚀中心向外腐蚀
		{
			
			x0=Result.x;
			y0=Result.y;
			
			Resu->x=Result.x;
			Resu->y=Result.y;
			Resu->w=Result.w;
			Resu->h=Result.h;
			flag=1;

			Area.X_Start= Result.x - ((Result.w)/2);
			Area.X_End  = Result.x + ((Result.w)/2);
			Area.Y_Start= Result.y - ((Result.h)/2);
			Area.Y_End  = Result.y + ((Result.h)/2);



			return 1;
		}
	else
		{
			
			flag=0;
			return 0;
		}

}





