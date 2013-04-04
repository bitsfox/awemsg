/*本程序用于获取在awesome的panel上输入显示信息，信息的获得使用原conky中个人编写的取得当地天气预报的数据
 在添加部分系统监视数据，定时一并输出显示到panel上。
 						tybitsfox  2013-4-3
 */
#include"awemsg.h"
//{{{ int main(int argc,char** argv)
int main(int argc,char** argv)
{
	char* av[]={weather,0};
	int i,j,job[jc];
	static int k=0;
	if(chg_daemon()!=0)
		exit(0);
	openlog(argv[0],LOG_PID,LOG_USER);
	syslog(LOG_NOTICE,"awesome_panel write tool ready\n");
	closelog();
	get_config();
	//测试对cpu和mem的获取使用内存映射
	/*if(crt_mmap()!=0)
	{
		openlog(argv[0],LOG_PID,LOG_USER);
		syslog(LOG_NOTICE,"create mmap error\n");
		closelog();
		clear_own();
		exit(0);
	}*/
	//为保证正常获取，执行三次
	for(i=0;i<3;i++)
	{
		//if(execvp(weather,av)==-1)
		if(system(weather)==-1)
		{
			openlog(argv[0],LOG_PID,LOG_USER);
			syslog(LOG_NOTICE,"error to get weather\n");
			closelog();
			sleep(1);
			continue;
		}
		else
			break;
	}
	format_msg(0);//初次运行时获取天气
	get_batt();// 初次运行时获取电量
	get_cpu();//cpu
	get_mem();//mem
	if(disp_msg()==0)
	{
		openlog(argv[0],LOG_PID,LOG_USER);
		syslog(LOG_NOTICE,fmt);
		closelog();
	}	
	else
	{
		openlog(argv[0],LOG_PID,LOG_USER);
		syslog(LOG_NOTICE,"show messge error\n");
		closelog();
		//clear_own();
		exit(0);
	}
	for(i=0;i<jc;i++)
		job[i]=0;
	//sleep(50);
	while(1)
	{
		sleep(2);
		for(i=0;i<jc;i++)
		{
			job[i]++;
			switch(i)
			{
				case 0://job 1
					if(job[0]>=tj[0].n)
					{
						job[0]=0;
						//execvp(weather,av);
						system(weather);
						format_msg(0);
						k=1;
					}
					break;
				case 1: //job2-battery
					if(job[1]>=tj[1].n)
					{
						job[1]=0;
						get_batt();
						//format_msg(1); nouse here
						k=1;
					}
					break;
				case 2://cpu
					if(job[2]>=tj[2].n)
					{
						job[2]=0;
						get_cpu();
						k=1;
					}
					break;
				case 3://mem
					if(job[3]>=tj[3].n)
					{
						job[3]=0;
						get_mem();
						k=1;
					}
					break;
			};
		}
		if(k)
		{
			k=0;
			if(disp_msg()!=0)
			{
				openlog(argv[0],LOG_PID,LOG_USER);
				syslog(LOG_NOTICE,"show messge error#1\n");
				closelog();
		//		clear_own();
				exit(0);
			}
		}
/*		if(job[0]%10==0)
		{
				openlog(argv[0],LOG_PID,LOG_USER);
				syslog(LOG_NOTICE,"normal running\n");
				closelog();				
		}*/		
	}
	openlog(argv[0],LOG_PID,LOG_USER);
	syslog(LOG_NOTICE,"now testing over...\n");
	closelog();
	//clear_own();
	exit(0);
}//}}}
//{{{ int chg_daemon()
int chg_daemon()
{
	int i,pid;
	pid=fork();
	if(pid!=0)
		return 1;
	signal(SIGHUP,SIG_IGN);
	if(setsid()<0)
		return 1;
	pid=fork();
	if(pid!=0)
		return 1;
	chdir("/");
	for(i=0;i<64;i++)
		close(i);
	open("/dev/null",O_RDONLY);
	open("/dev/null",O_RDWR);
	open("/dev/null",O_RDWR);
	return 0;
}//}}}
//{{{ void get_config()
void get_config()
{
	int i;
//初始化所有指针，句柄，描述字
	tj[0].n=3601; //0为天气的资料获取索引，轮寻时间为3600秒、1小时
	tj[1].n=15;  //1为电池电量的获取索引，轮寻时间为20秒
	tj[2].n=3; //2为CPU频率获取索引，轮寻时间为6秒
	tj[3].n=3; //3为内存使用索引
	for(i=0;i<4;i++)
		cpu_v[i]=0;
}
//}}}
//{{{ void format_msg(int i)
void format_msg(int i)
{
	FILE *f;
	int m;
	char buf1[chlen];
	if(i==0)
	{
		memset(msg[i],0,100);
		memset(msg[i+1],0,100);
		f=fopen(wfile,"r");
		if(f==NULL)
		{
			memcpy(msg[0],"Unknow",6);
			memcpy(msg[1],"Unknow",6);
			return;
		}
		fgets(msg[0],sizeof(msg[0]),f);//温度
		m=strlen(msg[0]);
		if(msg[0][m-1]=='\n')
			msg[0][m-1]=0;
		fgets(msg[1],sizeof(msg[1]),f);//今日天气
		m=strlen(msg[1]);
		if(msg[1][m-1]=='\n')
			msg[1][m-1]=0;
		fclose(f);
		return;
	}
	if(i==1)
	{
		return;
	}
	return;
}
//}}}
//{{{ int disp_msg()
int disp_msg()
{
	FILE *f;
	int i;
	f=popen(awesome,"w");
	if(f==NULL)
		return 1;
	memset(fmt,0,chlen);
	snprintf(fmt,chlen,out_msg,"#00ffff",msg[5],msg[4],msg[3],msg[2],msg[1],msg[0]);
	fputs(fmt,f);
	pclose(f);
	return 0;
}//}}}
//{{{ void get_batt();//battery get
void get_batt()//battery get
{
    float ft;
    FILE *f;
    int i,j,k,l;
    char *c,ch[512];
	memset(msg[2],0,100);
    f=fopen(sfile,"r");
    if(f==NULL)
    {
		memcpy(msg[2],"55",3);
        //printf("err");
        return ;
    }
    i=j=0;k=0;
    memset(ch,0,sizeof(ch));
    while(fgets(ch,sizeof(ch),f)!=NULL)
    {
        c=strstr(ch,power_base);
        if(c!=NULL)
        {
            c+=strlen(power_base);
            //printf("%s",c);
            i=atoi(c);
            k++;
            goto  lop1;
        }
        c=strstr(ch,power_now);
        if(c!=NULL)
        {
            c+=strlen(power_now);
            //printf("%s",c);
            j=atoi(c);
            k++;
        }
lop1:
        memset(ch,0,sizeof(ch));
        if(k>=2)
            break;
    }
    fclose(f);
    ft=(float)j/i;
    snprintf(msg[2],100,"%2.0f%% ",ft*100);
    return ;

}//}}}
//{{{ void clear_own()
void clear_own()
{
	if(pipef!=NULL)
		pclose(pipef);
	if(cpu_area!=NULL)
		munmap(cpu_area,mem_len);
	if(mem_area!=NULL)
		munmap(mem_area,mem_len);
	if(cpufd!=-1)
		close(cpufd);
	if(memfd!=-1)
		close(memfd);
	return;
}//}}}
//{{{ int crt_mmap()
int crt_mmap()
{
	cpufd=open(cpu_file,O_RDONLY);
	if(cpufd==-1)
		return 1;
	cpu_area=mmap(NULL,mem_len,PROT_READ,MAP_PRIVATE,cpufd,0);
	if(cpu_area==MAP_FAILED)
		return 1;
	memfd=open(mem_file,O_RDONLY);
	if(memfd==-1)
		return 1;
	mem_area=mmap(NULL,mem_len,PROT_READ,MAP_PRIVATE,memfd,0);
	if(mem_area==MAP_FAILED)
		return 1;
	return 0;
}//}}}
//{{{ void get_cpu()
void get_cpu()
{
	float fot;
	FILE *f;
	int len,i,j,k[4],t[4],l;
	char ch[mem_len],*c1,*c2,buf[10];
	memset(msg[5],0,100);
	f=fopen(cpu_file,"r");
	if(f==NULL)
	{
		snprintf(msg[5],100,"Unknow");
		return;
	}
	memset(ch,0,mem_len);
	fgets(ch,mem_len,f);
	fclose(f);
	c1=ch;len=strlen(ch);
	for(i=0;i<4;i++)
	{
		len=strlen(c1);
		for(j=0;j<len;j++)
		{
			if(c1[j]>=0x30 && c1[j]<=0x39)
			{
				l=1;
				while(c1[j+l]!=' ')
				{
					l++;
					if(l>=(len-j))
					{
						l=-1;
						break;
					}
				}
				if(l==-1)
				{
					snprintf(msg[5],100,"ooo");
					return;
				}
				memset(buf,0,10);
				memcpy(buf,&c1[j],l);
				k[i]=atoi(buf);
				c2=c1+l+j;
				c1=c2;
				break;
			}
		}
	}
	for(i=0;i<4;i++)
	{
		t[i]=k[i]-cpu_v[i];
		cpu_v[i]=k[i];
	}
	fot=(float)t[0]/(t[0]+t[1]+t[2]+t[3]);
	snprintf(msg[5],100,"%2.0f%% ",fot*100);
	return;
}//}}}
//{{{ void get_mem()
void get_mem()
{
	FILE *file;
	int a,i,j,k[2],l;
	char c[2][100],buf[20],*c1,*c2,ch[100];
	float fot;
	memset(msg[3],0,100);
	memset(msg[4],0,100);
	file=fopen(mem_file,"r");
	if(file==NULL)
	{
		snprintf(msg[3],100,"xx");
		snprintf(msg[4],100,"oo");
		return;
	}
	for(i=0;i<6;i++)
	{
		memset(ch,0,100);
		fgets(ch,100,file);
		if(i==0)
			memcpy(c[0],ch,100);
		if(i==5)
			memcpy(c[1],ch,100);
	}
	for(a=0;a<1;a++)
	{
		l=strlen(c[a]);c1=c[a];
		for(i=0;i<l;i++)
		{
			if(c[a][i]>=0x30 && c[a][i]<=0x39)
			{
				c1=c[a]+i;
				break;
			}
		}
		l=strlen(c1);j=0;
		for(i=0;i<l;i++)
		{
			if(c1[i]==' ')
			{
				j=1;
				memset(buf,0,20);
				memcpy(buf,c1,i);
				k[a]=atoi(buf);
				break;
			}
		}
		if(j==0)
		{
			snprintf(msg[3],100,"xx");
			snprintf(msg[4],100,"oo");
			return;
		}
	}
	fot=(float)k[1]/1024;
	snprintf(msg[4],100,"%0.1fMb ",fot);
	fot=(float)k[1]/k[0];
	snprintf(msg[3],100,"%2.2f%% ",fot*100);
	return;
}//}}}
//{{{ void get_net()
void get_net()
{
	return;
}//}}}
