#include<graphics.h>
#include<time.h>
#include "tools.h"
#include "vector2.h"
#include<mmsystem.h>
#include <string>
#include <iostream>
using namespace std;
#pragma comment(lib,"winmm.lib")

#define WIN_WIDTH 900
#define WIN_HEIGHT 600

char card[8][20] =
{
	{"pea"},{"sunflower"},{"cherrybomb"},{"chomper"},
	{"hypnoshroom"},{"iceshroom"},{"jalapeno"},{"potatomine"}
};

IMAGE imgBg;
IMAGE imgBar;
IMAGE img_card[8];
IMAGE* img_zhiwu[2][20];

int curX, curY;//当前选中的植物，在移动过程中的位置
int curzhiwu; //0没有选择，1选择一种植物

struct zhiwu {
	int type;     //0没有选择，1选择一种植物
	int frameIndex;//序列帧的序号

	int timer;
	bool catched;//是否被抓
	int blood;

	int x, y;
};
struct zhiwu map[3][9];

enum { SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_RPODUCT };
struct sunshineBall {
	int x, y;//阳光飘落位置的坐标（x不变）
	int frameIndex;//当前显示图片帧的序号
	int destY;//飘落目标位置的坐标
	bool used;//是否在使用
	int timer;

	float xoff;
	float yoff;

	float t;//贝塞尔时间点0.1
	vector2 p1, p2, p3, p4;
	vector2 pCur;//阳光球位置
	float speed;
	int status;
};
//预先准备阳光池，用完归还到池子
struct sunshineBall balls[10];
IMAGE imgsunshineBall[29];
static int sunshine;

struct zm {
	int x, y;
	int frameIndex;
	bool used;//是否出场

	int speed;
	int row;

	int blood;

	bool dead;
	bool eating;
};
struct zm zms[10];
IMAGE imgZM[22];
IMAGE imgZMDead[20];
IMAGE imgZMEat[21];

struct bullet {
	int x, y;
	int row;
	int used;
	int speed;
	bool blast;//是否发生爆炸
	int frameIndex;
};
struct bullet bullets[30];
IMAGE imgBulletNormal;
IMAGE imgBlast[4];


bool fileExist(const char* name) {
	FILE* fp = fopen(name, "r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}
void startUI() {
	//开始菜单

	//加载背景图片
	IMAGE imgBg, imgmenu1, imgmenu2;
	loadimage(&imgBg, "res/menu.png");
	loadimage(&imgmenu1, "res/menu1.png");
	loadimage(&imgmenu2, "res/menu2.png");

	int flag = 0;

    putimage(0, 0, &imgBg);

	//阻塞式等待
	while (1) {
		BeginBatchDraw();//双缓冲
		putimagePNG(475, 75, flag ? &imgmenu2 : &imgmenu1);

		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN &&
				msg.x > 474 && msg.y < 474 + 300 &&
				msg.y > 75 && msg.y < 75 + 140) {
				flag = 1;
			}
			else if (flag = 1 && msg.message == WM_LBUTTONUP) {
				return;
			}
		}
		EndBatchDraw();
	}
}


char name[64];
void gameInit() {
	loadimage(&imgBg, "res/bg.jpg");
	loadimage(&imgBar, "res/bar.png");

	memset(img_zhiwu, 0, sizeof(img_zhiwu));
	memset(map, 0, sizeof(map));

	string str1 = "res/Cards/card_";
	string str2 = ".png";
	string str3;
	for (int i = 0; i < 8; i++) {
		str3 = str1 + string(card[i]) + str2;

		sprintf_s(name, sizeof(name), str3.c_str());
		loadimage(&img_card[i], name);
	}

	

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 20; j++) {
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			if (fileExist(name)) {
				img_zhiwu[i][j] = new IMAGE;//分配内存
				loadimage(img_zhiwu[i][j], name);
			}
			else {
				break;
			}
		}
	}
	curzhiwu = 0;
	sunshine = 50;
	//初始化窗口
	initgraph(WIN_WIDTH, WIN_HEIGHT, 1);

	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgsunshineBall[i], name);
	}

	//配置时间种子
	srand(time(NULL));

	//设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;//抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT);//设置背景
	setcolor(BLACK);//文本颜色

	//初始化僵尸
	memset(zms, 0, sizeof(zms));
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZM[i], name);
	}
	
	loadimage(&imgBulletNormal, "res/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));

	//豌豆爆炸动画
	loadimage(&imgBlast[3], "res/bullet_blast.png");
	for (int i = 0; i < 3; i++) {
		float k = (i + 1) * 0.2;
		loadimage(&imgBlast[i], "res/bullet_blast.png",
			imgBlast[3].getwidth() * k,
			imgBlast[3].getheight() * k, true);
	}

	for (int i = 0; i < 20; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZMDead[i], name);
	}

	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZMEat[i], name);
	}
}

void drawfist() {
	putimage(0, 0, &imgBg);
	putimagePNG(250, 0, &imgBar);//渲染背景

	for (int i = 0; i < 8; i++) {
		int x = 338 + i * 65;
		int y = 6;
		putimagePNG(x, y, &img_card[i]);//渲染植物卡牌图片
	}
}

void createSunshine() {
	static int count = 0;
	static int fre = 200;
	count++;
	if (count >= fre) {
		fre = 100 + rand() % 100;
		count = 0;
		int ballMax = sizeof(balls) / sizeof(balls[0]);

		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax)return;

		balls[i].used = true;
		balls[i].frameIndex = 0;
		balls[i].timer = 0;

		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(260 + rand() % (900 - 260), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		int off = 2;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);
	}

	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == 2) {
				map[i][j].timer++;
				if (map[i][j].timer > 280 + rand() % 200) {
					map[i][j].timer = 0;
					int k = 0;
					for (; k < ballMax && balls[k].used; k++);
					if (k >= ballMax) return;
				
					balls[k].used = true;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (65 + rand() % 20) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w,
						map[i][j].y + img_zhiwu[1][0]->getheight()
						- imgsunshineBall[0].getheight());
					balls[k].p2 = vector2(balls[i].p1.x + w * 0.3, balls[i].p1.y - 100);
					balls[k].p3 = vector2(balls[i].p1.x + w * 0.7, balls[i].p1.y - 100);
					balls[k].status = SUNSHINE_RPODUCT;
					balls[k].speed = 0.05;
					balls[k].t = 0;
				}
			}
		}
	}
}

void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >= 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer > 100) {
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t > 1) {
					sun->used = false;
					sunshine += 25;
				}
			}
			else if (balls[i].status == SUNSHINE_RPODUCT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t > 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}

			/*if (balls[i].timer == 0) {
				balls[i].y += 2;
			}
			if (balls[i].y >= balls[i].destY) {
				balls[i].timer++;
				if (balls[i].timer > 100) {
					balls[i].used = false;
				}
			}*/
		}
	}
}

void drawSunshines() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			IMAGE* img = &imgsunshineBall[balls[i].frameIndex];
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}
}
void collectSunshine(ExMessage* msg) {
	int count = sizeof(balls) / sizeof(balls[0]);
	int w = imgsunshineBall[0].getwidth();
	int h = imgsunshineBall[0].getheight();
	for (int i = 0; i < count; i++) {
		if (balls[i].used) {
			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;
			if (msg->x > x && msg->x < x + w &&
				msg->y >y && msg->y < y + h) {
				//balls[i].used = false;
				balls[i].status = SUNSHINE_COLLECT;
				PlaySound("res/ptcm2-7mmwu.wav", NULL, SND_FILENAME | SND_ASYNC);
				balls[i].p1 = balls[i].pCur;
				balls[i].p4 = vector2(262, 0);
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);
				float off = 8;
				balls[i].speed = 1.0 / (distance / off);
			}
		}
	}
}
void draw_bg_zhiwu() {
	putimage(0, 0, &imgBg);
	putimagePNG(250, 0, &imgBar);//渲染背景

	for (int i = 0; i < 8; i++) {
		int x = 338 + i * 65;
		int y = 6;
		putimagePNG(x, y, &img_card[i]);//渲染植物卡牌图片
	}

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				/*int x = 256 + j * 81;
				int y = 179 + i * 102 + 14;*/
				int zhiwutype = map[i][j].type - 1;//渲染植物的类型
				int index = map[i][j].frameIndex;//渲染植物的状态
				putimagePNG(map[i][j].x, map[i][j].y, img_zhiwu[zhiwutype][index]);//植物种下的渲染
			}
		}
	}

	if (curzhiwu > 0) {
		IMAGE* img = img_zhiwu[curzhiwu - 1][0];
		//运动过程植物渲染
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}
}
void drawZM() {
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used) {
			//IMAGE* img = &imgZM[zms[i].frameIndex];
			//IMAGE* img = (zms[i].dead) ? imgZMDead : imgZM;
			IMAGE* img = NULL;

			if (zms[i].dead)img = imgZMDead;
			else if (zms[i].eating)img = imgZMEat;
			else img = imgZM;

			img += zms[i].frameIndex;

			putimagePNG(
				zms[i].x,
				zms[i].y - img->getheight(),
				img);
		}
	}
}
void drawbullet() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			if (bullets[i].blast) {
				IMAGE* img = &imgBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else
			{
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}

void updataZhiwu() {
	//植物摆动时图片帧的变化
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				map[i][j].frameIndex++;//种下后植物运动

				int zhiwuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;

				if (img_zhiwu[zhiwuType][index] == NULL) {
					map[i][j].frameIndex = 0;//重新进入循环，继续摆动
				}
			}
		}
	}
}
void createZM() {
	static int zmfre = 80;
	static int count = 0;

	count++;
	if (count > zmfre) {
		count = 0;
		zmfre = rand() % 200 + 200;

		int i;
		int zmMAX = sizeof(zms) / sizeof(zms[0]);
		for (i = 0; i < zmMAX && zms[i].used; i++);
		if (i < zmMAX) {
			memset(&zms[i], 0, sizeof(zms[0]));
			zms[i].used = true;
			zms[i].x = WIN_WIDTH + 10;
			zms[i].row = rand() % 2;
			zms[i].y = 172 + (1 + zms[i].row) * 100;
			zms[i].speed = 1;
			zms[i].blood = 100;
			zms[i].dead = false;
		}
		else {
			printf("创建失败！\n");
		}

	}
}
void updateZM() {
	int zmMAX = sizeof(zms) / sizeof(zms[0]);

	static int count = 0;
	count++;
	if (count >= 2) {
		count = 0;
		//更新僵尸位置
		for (int i = 0; i < zmMAX; i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;//僵尸速度
				if (zms[i].x < 170) {
					printf("GAME OVER小黑子！\n");
					MessageBox(NULL, "over", "over", 0);
					exit(0);
				}
			}
		}
	}

	static int count2 = 0;
	count2++;
	if (count2 > 4) {
		count2 = 0;
		for (int i = 0; i < zmMAX; i++) {
			if (zms[i].used) {
				if (zms[i].dead) {
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20) {
						zms[i].used = false;
					}
				}
				else if (zms[i].eating) {
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
				}
				else {
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;//动作变换频率
				}


			}
		}
	}
}
void shoot() {
	int lines[3] = { 0 };
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerx = WIN_WIDTH - imgZM[0].getwidth();

	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used && zms[i].x < dangerx) {
			lines[zms[i].row] = 1;//有无僵尸
		}
	}

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == 1 && lines[i]) {
				static int count = 20;
				count++;
				if (count > 50) {
					count = 0;

					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);
					if (k < bulletMax) {
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 6;

						bullets[k].blast = false;
						bullets[k].frameIndex = 0;

						int zwX = 256 + j * 81;
						int zwY = 179 + i * 102 + 14;
						bullets[k].x = zwX + img_zhiwu[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = zwY + 5;
					}
				}
			}
		}
	}
}
void updateBullets() {
	int countMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < countMax; i++) {
		if (bullets[i].used) {
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > WIN_WIDTH) {
				bullets[i].used = false;
			}

			//碰撞检测
			if (bullets[i].blast) {
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex >= 4) {
					bullets[i].used = false;
				}
			}
		}
	}
}
void checkBZM() {
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < bCount; i++) {
		if (bullets[i].used == false || bullets[i].blast)continue;

		for (int k = 0; k < zCount; k++) {
			if (zms[k].used == false) continue;

			int x1 = zms[k].x + 80;
			int x2 = zms[k].x + 110;
			int x = bullets[i].x;
			if (zms[k].dead == false && bullets[i].row == zms[k].row && x > x1 && x < x2) {
				zms[k].blood -= 10;
				bullets[i].blast = true;
				bullets[i].speed = 0;

				if (zms[k].blood <= 0) {
					zms[k].dead = true;
					zms[k].speed = 0;
					zms[k].frameIndex = 0;
				}
				break;
			}
		}
	}
}

void checkZZM() {
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].dead)continue;

		int row = zms[i].row;
		for (int k = 0; k < 9; k++) {
			if (map[row][k].type == 0)continue;

			int zhiwuX = 256 + k * 81;
			int x1 = zhiwuX + 10;
			int x2 = zhiwuX + 60;
			int x3 = zms[i].x + 80;
			if (x3 > x1 && x3 < x2) {
				if (map[row][k].catched) {
					map[row][k].blood -= 1;
					if (map[row][k].blood == 0) {
						map[row][k].type = 0;
						zms[i].eating = false;
						zms[i].frameIndex = 0;
						zms[i].speed = 1;
					}
				}
				else {
					map[row][k].catched = true;
					zms[i].eating = true;
					zms[i].speed = 0;
					zms[i].frameIndex = 0;
				}
			}
		}
	}
}
void userclick() {
	ExMessage msg;
	static int status = 0;
	static int status2 = 0;

	if (peekmessage(&msg)) {
		if (msg.message == WM_LBUTTONDOWN) {
			//判断点击时选择bar中哪种植物
			if (msg.x > 338 && msg.x < 338 + 65 * 2 && msg.y < 96) {
				int index = (msg.x - 338) / 65;
				status = 1;//若选择范围正确状态为1
				curzhiwu = 1 + index;//选择植物类型
				if (curzhiwu == 1) {
					status2 = 100;
				}
				else if (curzhiwu == 2) {
					status2 = 50;
				}
			}
			else {
				collectSunshine(&msg);
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1) {
			//移动时鼠标坐标
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP) {
			if (sunshine >= status2) {
				//松开时种下植物
				if (msg.x > 256 && msg.y > 179 && msg.y < 494) {
					int row = (msg.y - 179) / 105;
					int col = (msg.x - 256) / 81;
					//判断种下位置
					if (map[row][col].type == 0) {
						map[row][col].type = curzhiwu;
						map[row][col].frameIndex = 0;
						map[row][col].blood = 120;
						map[row][col].x = 256 + col * 81;
						map[row][col].y = 179 + row * 105;
						sunshine -= status2;
					}
				}
			}
			curzhiwu = 0;//curzhiwu恢复无选中状态
			status = 0;//状态重新初始为0
			status2 = 0;
		}
	}
}


void updateWindow() {
	BeginBatchDraw();//开始缓冲
	//背景&摆动植物
	/*
	At first, render the basic background,including cards of plants ,background images,the value of sunlight 
	and the plants which swinging in the grass square grid.
	*/
	draw_bg_zhiwu();

	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(276, 67, scoreText);//输出阳光值

	/*
	And then draw the bullets fired by the Pea Shooter,the sunshine and the corpse.
	Sunshine has 4 states: Falling from above, Falling on the ground, Being produced by sunflowers, Being collected
	Corpse has 3 status: Eating, Walking Dead
	Bullets has 2 status: Moving, Exploding
	This module is to render them in different states
	*/
	drawSunshines();
	drawZM();
	drawbullet();

	EndBatchDraw();//结束双缓冲
}


void updateGame() {
	/*
	Randomly generate sunlight, zombies, and bullets and update the status of them
	*/
	updataZhiwu();

	createSunshine();
	/*
	What deserves to be mentioned is that Sunflowers generating sunshine uses the Bezier curve 
	through which we can create a fitting curve based on several points.
	I will show you a vedio here
	*/
	updateSunshine();
	
	createZM();
	updateZM();

	shoot();
	updateBullets();
	/*
	Check if zombies collide with plants and if bullets collide with zombies. 
	And determine the status of the above three based on the detection results.
	*/
	checkBZM();
	checkZZM();
}


int main(void) {
	//加载图片，初始化数据
	//This function is used to Load data and initialize data
	//Transfer data stored on the hard drive to the heap area
	gameInit();
	//初始化界面
	//and then Display initialization interface
	startUI();
	//初步展示游戏界面
	//Preliminarily show the game interface
	drawfist();
	int timer = 0;
	bool flag= true;

	while (1) {
	//循环

		//接受用户鼠标信息并响应
		//This module is for receiving user mouse information and respond
		userclick();
		//目的在于对页输出进行缓冲处理
		//The purpose of this operation is to buffer the page output,
		//This makes the program smoother and improves the user experience
		timer += getDelay();
		
		if (timer > 20) {
			flag = true;
			timer = 0;
		}
		if (flag) {
			flag = false;
			/*
			The following two modules are the most important, 
			aimed at rendering game windows and updating game data,
			and are the core of the program
			*/
			updateWindow();//渲染游戏窗口
			updateGame();//更新游戏数据
		}
	}
	system("pause");
	return 0;
}