// Asteroids. (c) Dice.com 2012. AUthor D. Bolton
// Version 1.3 Updated October 14,2017 - switched to SDL2

#include <time.h>
#include "SDL.h"   /* All SDL App's need this */
#include "SDL_image.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// #defines
#define WIDTH 1024
#define HEIGHT 768
#define NUMTEXTURES 11
#define SHIPWIDTH 64
#define SHIPHEIGHT 64
#define PLBACKDROP 0
#define TEXTTEXTURE 1
#define TXSMALLSHIP 2
#define TXASTEROID1 3
#define TXDEBUG 7
#define TXSHIP 8
#define TXEXPLOSION 9
#define PLBULLETS 10
#define MAXASTEROIDS 255
#define GRHEIGHT 48
#define GRWIDTH 48
#define BLINKPERIOD 450
#define DEMOTIMESECS 30
#define MAXHISCORES 10
#define MAXBULLETS 16
#define MAXLEN 3
#define MAXEXPLOSIONS 255

#define SNEMPTY -1

#define QUITKEY SDLK_ESCAPE
#define TESTKEY SDLK_t
#define TOGGLEFPS SDLK_f
#define PAUSEKEY SDLK_p
#define DEBUGKEY SDLK_TAB
#define FIFTYWIDTH 34
#define FIFTYHEIGHT 18
#define BANGKEY SDLK_b
#define COUNTERCLOCKWISE SDLK_q
#define CLOCKWISE SDLK_w
#define THRUSTKEY SDLK_LCTRL
#define HYPERJUMPKEY SDLK_h
#define FIREKEY SDLK_SPACE

struct asteroid {
	int x,y;
	int active;
	int xvel,yvel;
	int xtime,ytime;
	int xcount,ycount;
	int rotclockwise;
	int rottime;
	int rotcount;
	int rotdir;
	int size;
};

struct hiscore {
	int value;
	char initials[4];
};

struct bullet {
	float x,y;
	int countdown;
	int timer;
	float vx,vy;
	int ttl;
};

struct explosion {
	int x,y;
	int countdown;
	int timer;
	int phase;
};

struct player {
	float x,y;
	int dir; /* 0-23 */
	float vx,vy;
	int lives;
};




// /Global variables	
	const char * texturenames[NUMTEXTURES]= {"assets/starfield.png","assets/text.png","assets/smallship.png","assets/a1.png",
		"assets/a2.png","assets/a3.png","assets/a4.png",		"assets/debug.png","assets/playership.png",
		"assets/explosion.png","assets/bullet.png" };
	const int sizes[4]= {280,140,70,35};
	const int debugOffs[4]={0,280,420,490};
	const int xdir[8]= {0,1,1,1,0,-1,-1,-1};
	const int ydir[8]= {-1,-1,0,1,1,1,0,-1};	
	const int bulletx[24]={32,38,50,64,66,67,68,67,66,64,50,38,32,26,16, 0,-2,-3,-4,-3,-2,0,16,24};
	const int bullety[24]={-4,-3,-2, 0,16,26,32,38,50,64,66,67,71,70,69,64 ,50,38,32,26,16,0,-2,-3};

	SDL_Texture* textures[NUMTEXTURES];
	SDL_Window* screen = NULL;	 
	SDL_Renderer *sdlRenderer;
	SDL_Texture * texture;
	SDL_Event event;	
	SDL_Rect source,destination;
	SDL_Rect dst;
	float thrustx[24];
	float thrusty[24];
	int gameRunning = 1;
	int rotateFlag; /* 0 do mowt, 1 counter clockwise, 2 clockwise */
	int debug=0;
	int speedTimer,thrustFlag,hyperJump,jumpTimer,fireTimer,fireFlag;
	int keypressed;
	int numImages,numAsteroids,score,gameExited,inHighScore,numOnScreen;
	int framecount,fruittrapcount,movedir;
	int numsegs,tailindex,headindex,headloc,paused,pauseblink,blinktime;
	int loopTimer,rotTimer;

	struct asteroid asteroids[MAXASTEROIDS];
	struct bullet bullets[MAXBULLETS];
	struct hiscore hiscores[MAXHISCORES];
	struct explosion explosions[MAXEXPLOSIONS];
	struct player Player;
	int fps,tickcount,lasttick,errorCount;
	char buffer[50];
	char kbuffer[MAXLEN+1];
	int showfps=0;
	char gameovermsg[100];

/* returns a number between 1 and max */
int Random(int max) {
	return (rand() % max) + 1;
}

/* Sets Window caption according to state - eg in debug mode or showing fps */
void SetCaption() {
  if (paused==1)
	{
	  SDL_SetWindowTitle(screen, "Asteroids Game is Paused" );
	}
  else
	if (showfps) {
		sprintf(buffer,"Asteroids Game - %i fps and %i Asteroids",framecount,numAsteroids);
		SDL_SetWindowTitle(screen, buffer );
  }
	else
		SDL_SetWindowTitle(screen, "Asteroids Game" );
}

/* Initialize all asteroid variables */
void InitAsteroids() {
	score=0;
	for (int i=0;i<MAXASTEROIDS;i++) {
		asteroids[i].active =0;
	}
}

void LogError(char * msg) {
	//FILE * err;
	//int error;
    //error = fopen_s(&err,"errorlog.txt", "a");
	printf("%s\n",msg);
	//fclose(err);
	errorCount++;
}

void LogError2(const char * msg1, const char * msg2) {
	//FILE * err;
	//int error;
	//error = fopen_s(&err, "errorlog.txt", "a");
	//fprintf_s(err, msg1);
	printf("%s %s\n",msg1,msg2);
	//fclose(err);
	errorCount++;
}

SDL_Texture* loadTexture(const char * afile, SDL_Renderer *ren) {
	SDL_Texture *texture = IMG_LoadTexture(ren,afile);
	if (texture == 0) {
		LogError2("Failed to load texture from ", afile);
	}
	return texture;
}

/* Loads Textures */
void LoadTextures() {
	for (int i = 0; i<NUMTEXTURES; i++) {
		textures[i] = loadTexture(texturenames[i], sdlRenderer);
	}
}

void renderTexture(SDL_Texture *tex, int x, int y) {
	//Setup the destination rectangle to be at the position we want

	dst.x = x;
	dst.y = y;
	//Query the texture to get its width and height to use
	SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
	SDL_RenderCopy(sdlRenderer, tex, NULL, &dst);
}

void renderTextureRect(SDL_Texture *tex, SDL_Rect * dst) {
	//Setup the destination rectangle to be at the position we want
	SDL_RenderCopy(sdlRenderer, tex, NULL, dst);
}

// Replaces old SDL_BlitSurface as using textures
void blit(SDL_Texture *tex, SDL_Rect * src, SDL_Rect * dst) {
	SDL_RenderCopy(sdlRenderer, tex, src, dst);

}// Replaces old SDL_BlitSurface as using textures
void blitAll(SDL_Texture *tex,  SDL_Rect * dst) {
	SDL_RenderCopy(sdlRenderer, tex, 0, dst);
}




/* Initialize all setup, set screen mode, load images etc */ 
void InitSetup() {
	errorCount = 0;
	srand((int)time(NULL));
	SDL_Init( SDL_INIT_EVERYTHING );    
	SDL_CreateWindowAndRenderer(1024,768, SDL_WINDOW_SHOWN, &screen, &sdlRenderer);
	if (!screen) {
		LogError("InitSetup failed to create window");
	}
	SetCaption(); 
	LoadTextures();
}

/* Init all MAXSPRITES */
void InitBullets() {
	for(int i=0;i< MAXBULLETS;i++) {
		bullets[i].x=0;
		bullets[i].y=0;
		bullets[i].countdown=0;
		bullets[i].timer=0;
		bullets[i].ttl=0;
	}
}

/* Init all MAXSPRITES */
void InitExplosions() {
	for(int i=0;i< MAXEXPLOSIONS;i++) {
		explosions[i].x=0;
		explosions[i].y=0;
		explosions[i].countdown=0;
		explosions[i].timer=0;
		explosions[i].phase=-1;
	}
}

/* Init thrustx and thrusty[] */
void InitThrustAngles() {
	const float pi=3.14159265f;
	float degree;
	degree=0.0f;
	for (int i=0;i<24;i++) {
		thrustx[i]= (float)sin( degree*pi/180.0f);
		thrusty[i]= (float)-cos( degree*pi/180.0f);
		degree += 15;
	}
}

/* Initialize Player struct */
void InitPlayer() {
	Player.dir= 0;
	Player.vy=0;
	Player.vy=0;	
	Player.lives=3;
	Player.x= 500;
	Player.y= 360;
}


/* Intialize the game related stuff, called each time geame starts */
void InitGame() {
	paused=0;
	showfps=0;
	framecount=0;
	movedir=0;
	gameRunning = 1;
	numOnScreen=1;
	InitExplosions();
	InitAsteroids();
	InitPlayer();
	InitThrustAngles();

}

/* print char at rect target */
void printch(char c,SDL_Rect * target) {	
	SDL_Rect charblock;
	int start= (c-'!');
	if (c!= ' ') {
		charblock.h=23;
		charblock.w=12;
		charblock.x = start*12;
		charblock.y = 0;
		renderTextureRect(textures[TEXTTEXTURE],&charblock);
	}
	(*target).x+= 13;
}

/* print string text at x,y pixel coords */
void print(int x, int y, char * text) {
	SDL_Rect destr;
	destr.h = 23;
	destr.w = 12;
	destr.x = x;
	destr.y = y;
	for (int i = 0; i < (int)strlen(text); i++) {
		printch(text[i], &destr);
	}
}


/* Show score on screen */
void ShowScore() {
  SDL_Rect playerlives,destr;
  sprintf(buffer,"%i",score);
  print(25,GRHEIGHT*15,buffer);
  if (debug)
  {
	  print(250,GRHEIGHT*15,"Debug");
  }
  destr.h = 33;
  destr.w = 24;
  destr.y = 700;				
  playerlives.h=33;
  playerlives.w=24;		
  playerlives.x = 0;
  playerlives.y = 0;
  for (int i=0;i<Player.lives;i++) {  
	destr.x = (i*30)+50;
	renderTextureRect( textures[TXSMALLSHIP],&destr);
  }

}

/* Draw all explosions with countdown > 0 */
void DrawExplosions() {
	char buff[15];
	SDL_Rect spriterect,target;
	target.h = 64;		
	target.w = 64;
	spriterect.h=64;
	spriterect.w=64;		
	spriterect.y = 0;
	for (int i=0;i<MAXEXPLOSIONS;i++) {
		if (explosions[i].phase != -1) {
			target.x = explosions[i].x;
			target.y = explosions[i].y;
			spriterect.x=explosions[i].phase*64;
			blit(textures[TXEXPLOSION],&spriterect,&target);
			if (debug) {
			  sprintf(buff,"%i",explosions[i].phase);
			  print(explosions[i].x+64,explosions[i].y,buff);
			}
		}
	}
}

/* Draw all bullets with countdown > 0 */
void DrawBullets() {
	char buff[10];
	SDL_Rect spriterect,target;
	target.h = 3;		
	target.w = 3;
	spriterect.h=3;
	spriterect.w=3;		
	spriterect.x = 0;
	spriterect.y = 0;
	for (int i=0;i<MAXBULLETS;i++) {
		if (bullets[i].ttl >0) {
			target.x = (int)bullets[i].x;
			target.y = (int)bullets[i].y;
			blit(textures[PLBULLETS],&spriterect,&target);
			if (debug) {
			  sprintf(buff,"%i",bullets[i].ttl);
			  print((int)bullets[i].x+5,(int)bullets[i].y,buff);
			}
		}
	}
}
/* Draw All asteroids */
void DrawAsteroids() {
	int asize,sizeIndex;	
	char buff[30];
	SDL_Rect asteroidRect,target;
	numAsteroids=0;
	asteroidRect.y=0;
	for (int i=0;i<MAXASTEROIDS;i++) {
		if (asteroids[i].active) {	
			numAsteroids++;
			sizeIndex = asteroids[i].size;
			asize = sizes[sizeIndex];
			target.h = asize;		
			target.w = asize;
			asteroidRect.h=asize;
			asteroidRect.w=asize;	
			asteroidRect.x = asize*asteroids[i].rotdir;
			target.x = asteroids[i].x;
			target.y = asteroids[i].y;
			blit(textures[TXASTEROID1+sizeIndex],&asteroidRect,&target);
			if (!debug)
			  continue;
			asteroidRect.x= debugOffs[sizeIndex];
			blit(textures[TXDEBUG],&asteroidRect,&target);
			sprintf(buff,"%i",i);
			print(target.x+25,target.y+25,buff);
		}
	}
}

/* return index or -1 if not found */
int FindFreeAsteroidSlot() {
	int result=-1;
	int i;
	for (i=0;i< MAXASTEROIDS;i++) {
		if (!asteroids[i].active) {
			result = i;
			break;
		}
	}
	return result;
}


/* adds to table , size = 0..3*/
void AddExplosion(int x,int y) {
	int index=-1;
	for (int i=0;i<MAXEXPLOSIONS;i++) {
		if (explosions[i].phase==-1) {
			index=i;
			break;
		}
	}
	if (index==-1)
		return;

	explosions[index].phase=0;
	explosions[index].x=x;
	explosions[index].y= y;
	explosions[index].timer=1;
	explosions[index].countdown=1;
}

/* DestroyAsteroid, create 4 smaller plus 4 size 3 */
void DestroyAsteroid(int deadIndex) {
  int size,i,xvel,yvel,x,y,index,newsize;

  size =asteroids[deadIndex].size; 
  asteroids[deadIndex].active=0; 
  AddExplosion(asteroids[deadIndex].x,asteroids[deadIndex].y);
  if (size==3)
		  return; /* it's destroyed, not split so exit */
	xvel =asteroids[deadIndex].xvel; 
  yvel =asteroids[deadIndex].yvel;   
  x    =asteroids[deadIndex].x; 
  y    =asteroids[deadIndex].y; 
  for (i=0;i<8;i++) {
	  if (i%2==0) /* even i is size 3 */
		 newsize =3;
	  else
		 newsize=size+1; /* next smaller size */
	index = FindFreeAsteroidSlot();
	if (index==-1)
		return; /* no more asteroid slots */
	asteroids[index].active=1;
	asteroids[index].x=xdir[i]*sizes[size]+x;
	asteroids[index].y=ydir[i]*sizes[size]+y;
	asteroids[index].rotdir = 0;
	asteroids[index].rotclockwise = Random(2)-1;
	asteroids[index].xvel=xdir[i]*Random(4)+xvel;	  
	asteroids[index].yvel=ydir[i]*Random(4)+yvel;
	asteroids[index].xtime = 2 + Random(5);	
	asteroids[index].xcount = asteroids[index].xtime;
	asteroids[index].ytime = 2 + Random(5);
	asteroids[index].ycount = asteroids[index].ytime;
	asteroids[index].rottime = 2 + Random(8);
	asteroids[index].rotcount = asteroids[index].rottime;	 
	asteroids[index].size = newsize;	
  }
}

/* Draw player ship */
void DrawPlayer() {
	SDL_Rect spriterect,target;
	char buff[30];

	target.h = SHIPHEIGHT;		
	target.w = SHIPWIDTH;
	spriterect.h=SHIPHEIGHT;
	spriterect.w=SHIPWIDTH;		
	spriterect.x = Player.dir*SHIPWIDTH;
	spriterect.y = 0;
	target.x = (int)Player.x;
	target.y = (int)Player.y;
	blit(textures[TXSHIP],&spriterect,&target);
	if (!debug) return;
	/* code after here is only run if debug !=0 */
	target.w=96;
	target.h=96;

	spriterect.x=280;
	spriterect.y=140;
	spriterect.w=66;
	spriterect.h=66;
	blit(textures[TXDEBUG],&spriterect,&target);	
	sprintf(buff,"(%6.4f,%6.4f) Dir= %i",Player.x,Player.y,Player.dir);
	print((int)Player.x-50,(int)Player.y+66,buff);
	sprintf(buff,"(%6.4f,%6.4f)",Player.vx,Player.vy);
	print((int)Player.x-50,(int)Player.y+90,buff);
}

/* renders all graphics to buffer then flips it to screen */
void RenderScreen() {
	  renderTexture( textures[PLBACKDROP], 0, 0 );
	  DrawAsteroids();
	  DrawBullets();
	  DrawPlayer();
	  DrawExplosions();
	  ShowScore();  
	  SDL_RenderPresent(sdlRenderer);
	 //  SDL_Flip( screen ); SDL1
}

/* Cleans up after game over */
void FinishOff() {
	//Free the loaded image       
	for (int i=NUMTEXTURES-1;i>0;i--) 
	{ 
		SDL_DestroyTexture( textures[i] );
	}
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(screen);
	//Quit SDL
	SDL_Quit();
	exit(0);
}

/* adds to table , size = 0..3*/
void AddAsteroid(int size) {
	int index= FindFreeAsteroidSlot();
	if (index==-1) /* table full so quietly exit */
		return;

	asteroids[index].active=1;
	asteroids[index].x=Random(WIDTH-size)+20;
	asteroids[index].y= Random(HEIGHT-size)+20;
	asteroids[index].rotdir = 0;
	asteroids[index].rotclockwise = Random(2)-1;
	asteroids[index].xvel=4-Random(8);	  
	asteroids[index].yvel=4-Random(8);
	asteroids[index].xtime = 2+Random(5);	
	asteroids[index].xcount = asteroids[index].xtime;
	asteroids[index].ytime = 2+Random(5);
	asteroids[index].ycount = asteroids[index].ytime;
	asteroids[index].rottime = 2+Random(8);
	asteroids[index].rotcount = asteroids[index].rottime;	 
	asteroids[index].size = size;	
}

/* Move an Asteroid */
void MoveAsteroid(int index) {
	if (!asteroids[index].active)
		return;
  if (asteroids[index].xtime >0){
	asteroids[index].xtime--;
	if (asteroids[index].xtime<=0) {
		asteroids[index].x+= asteroids[index].xvel;
		asteroids[index].xtime= asteroids[index].xcount;
		if (asteroids[index].x < -sizes[asteroids[index].size] || asteroids[index].x > WIDTH) 
			asteroids[index].active =0;
	}
  }
  if (asteroids[index].ytime >0){
	asteroids[index].ytime--;
	if (asteroids[index].ytime<=0) {
		asteroids[index].y+= asteroids[index].yvel;
		asteroids[index].ytime= asteroids[index].ycount;		
		if (asteroids[index].y < -sizes[asteroids[index].size] || asteroids[index].y > HEIGHT) 
			asteroids[index].active=0;
	}
  }

	if (asteroids[index].rottime >0) {
		asteroids[index].rottime--;
		if (asteroids[index].rottime<=0) 
		{
			asteroids[index].rottime= asteroids[index].rotcount;
			if (asteroids[index].rotclockwise) {
					asteroids[index].rotdir = (asteroids[index].rotdir+1)%24;
				}
				else {
					asteroids[index].rotdir = ((asteroids[index].rotdir-1)+24)%24;
				}
		}
	}
}

/* check if this asteroid hit another */
int HitAsteroid(int index) {
	int i,x1,y1,x2,y2,x3,y3,x4,y4,xs1,xs2;
	if (!asteroids[index].active) /* check, might have been destroyed itself */
		return -1;
	x1 = asteroids[index].x;
	y1 = asteroids[index].y;
	xs1 = sizes[asteroids[index].size];
	x2 = x1 + xs1;
	y2 = y1 + xs1;	
	for (i=0;i<MAXASTEROIDS;i++) {
		if (i != index && asteroids[i].active) {
				x3 = asteroids[i].x;
				y3 = asteroids[i].y;
				xs2 = sizes[asteroids[i].size];
				x4 = x3 + xs2;
				y4 = y3 + xs2;	
				if (x3 > x2 || y2 < y3 || x4 < x1 || y4 < y1) 
					continue;
				return i;
		}
	}
	return -1;
}

/* check if player might hit an asteroid */
int PlayerHitAsteroid(int x1,int y1) {
	int i,x2,y2,x3,y3,x4,y4,xs2;
	x2 = x1 + 66;
	y2 = y1 + 66;	
	for (i=0;i<MAXASTEROIDS;i++) {
		if (asteroids[i].active) {
				x3 = asteroids[i].x;
				y3 = asteroids[i].y;
				xs2 = sizes[asteroids[i].size];
				x4 = x3 + xs2;
				y4 = y3 + xs2;	
				if (x3 > x2 || y2 < y3 || x4 < x1 || y4 < y1) 
					continue;
				return i;
		}
	}
	return -1;
}
/* Move All Asteroids */
void MoveAsteroids() {
	int i,oa;
	for (i=0;i<MAXASTEROIDS;i++) {
		{
			if (asteroids[i].active) {
				MoveAsteroid(i);
				oa= HitAsteroid(i);
				if (oa !=-1) {
					DestroyAsteroid(i);
					DestroyAsteroid(oa);
				}
			}
		}
	}
}

/* test code */
void BlowUpAsteroid() {
  int i;
   for (i=0;i<MAXASTEROIDS;i++) {
	   if (asteroids[i].active) {
		   DestroyAsteroid(i);
		   break;
	   }
   }
}
/* Handle all key presses except esc */
void ProcessKey() {

	switch (keypressed) {
	case TESTKEY: /* use for testing conditibbbbbbons */
		AddAsteroid(Random(4)-1);
		break;
	case PAUSEKEY: /* use to toggle pause */
		paused=1-paused;
		blinktime=SDL_GetTicks();
		pauseblink=1;
		SetCaption();
		break;	
	case COUNTERCLOCKWISE:
		rotateFlag=1;
		break;
	case CLOCKWISE:
		rotateFlag=2;	
		break;
	case DEBUGKEY: 
		debug = 1-debug;
		break;
	case THRUSTKEY:
		thrustFlag=1;
		break;
	case HYPERJUMPKEY:
		hyperJump=1;
		break;
	case  TOGGLEFPS:
		showfps = 1-showfps;
		SetCaption();
		keypressed=0;
		break;
	case FIREKEY:
		fireFlag=1;
		break;
	case BANGKEY:
		BlowUpAsteroid();
	}
}


/* read a character */
char getaChar() {
  int result =-1;

  while (SDL_PollEvent(&event)) {
	 if (event.type== SDL_KEYDOWN)
		 {
			 result= event.key.keysym.sym;
			 break;
		 }
  }
  return result;
}

/* move the player ship */
void MovePlayer() {
	Player.x += Player.vx;
	Player.y += Player.vy;	
	if (Player.x <=-5) 
		Player.x += 1024;
	else
		if (Player.x > 1020)
			Player.x=0;
	if (Player.y <=-5) 
		Player.y += 768;
	else
		if (Player.y > 764)
			Player.y=0;

}

/* doHyperJump() find empty space on screen*/
void doHyperJump() {
	int hx,hy;

	do {

		hx = 50+Random(WIDTH-100);
		hy = 50+Random(HEIGHT-100);
	}
	while (PlayerHitAsteroid(hx,hy) !=-1);
	Player.x=(float)hx;
	Player.y= (float)hy;
	Player.vx=0;
	Player.vy=0;
	jumpTimer= SDL_GetTicks();
}

/* fire a buller- first work out where to appear then add to bullets */
void doFireBullet() {
	int x,y,i,index;
	x = (int)round(Player.x + bulletx[Player.dir]);
	y = (int)round(Player.y + bullety[Player.dir]);
	index=-1;
	for (i=0;i<MAXBULLETS;i++) {
		if (bullets[i].ttl==0) { /* found a slot */
			index=i;
			break;
		}
	}
	if (index==-1) return; /* no free slots all bullets in play */ 
	bullets[index].ttl=120;
	bullets[index].x= x*1.0f;
	bullets[index].y = y*1.0f;
	bullets[index].countdown =1;
	bullets[index].timer=3;
	bullets[index].vx = Player.vx+thrustx[Player.dir]*5;
	bullets[index].vy = Player.vy+thrusty[Player.dir]*5;
}

/* Cycle Explosions through all the phases */
void CycleExplosions() {
 int i;
 for (i=0;i<MAXEXPLOSIONS;i++) {
	 if (explosions[i].phase >-1) {
		 if ( explosions[i].countdown>0) {
			explosions[i].countdown--;
			if (explosions[i].countdown==0) {
				explosions[i].phase++;
				if (explosions[i].phase==64) {
					explosions[i].phase=-1; /* bye bye bang */
					continue;
				}
				explosions[i].countdown=explosions[i].timer;
			}
		 }
	 }
 }
}

/* move bullets * */
void MoveBullets() {
	int i,index;
	float vx,vy;
	for (i=0;i< MAXBULLETS;i++) {
		if (bullets[i].countdown >0) {
			bullets[i].countdown--;
			if (bullets[i].countdown==0) {
				if (bullets[i].ttl >0) 
					{
						bullets[i].ttl--;
						if (bullets[i].ttl==0)
						  break; /* expired */
					}
				 /* move it */
				bullets[i].countdown=bullets[i].timer;
				vx = bullets[i].x + bullets[i].vx;
				vy = bullets[i].y + bullets[i].vy;			
				if (vx <=-2) /* check for screen edges */
					vx += 1024;
				else
					if (vx > 1022)
						vx=0;
				if (vy <=-2) 
					vy += 768;
				else
					if (vy > 766)
						vy=0;
				bullets[i].x=vx;
				bullets[i].y=vy;
				index= PlayerHitAsteroid((int)vx,(int)vy);
				if (index != -1) {
					{
						DestroyAsteroid(index);
						bullets[i].countdown=0; /* remove bullet */
						bullets[i].ttl=0;
					}
				}
			}
		}
	}
}

/* main game loop. Handles demo mode, high score and game play */
void GameLoop() {
	while (gameRunning)
	   {		  	  
		  framecount++;
		  tickcount = SDL_GetTicks();

		  if (tickcount - lasttick >= 1000 && showfps) {		  
				lasttick = tickcount;
				SetCaption();				
				framecount =0;
		  }
		  if (paused && ((SDL_GetTicks()-blinktime)> BLINKPERIOD)) {
			  pauseblink = 1-pauseblink;
			  blinktime = SDL_GetTicks();
		  }
		  while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					keypressed = event.key.keysym.sym;
					if (keypressed == QUITKEY)
					{
						strcpy(gameovermsg,"User hit the ESC Key");
						gameRunning=0;
						gameExited=1;
						goto exitgame;
					}

					ProcessKey();
					break;
				case SDL_QUIT: /* if mouse clkck to close window */
					{
						gameRunning=0;
						gameExited=1;
						goto exitgame;
						break;
					}
				case SDL_KEYUP: {
					rotateFlag=0;
					thrustFlag=0;
					fireFlag=0;
					break;
				}
			} /* switch */
		  
			} /* while SDL_PollEvent */				
		  if (rotateFlag && (SDL_GetTicks()-rotTimer > 40)) {
			rotTimer=SDL_GetTicks();
			if (rotateFlag==1) /* CounterClockwise */
			{
				Player.dir+=23;
				Player.dir %=24;
			}
			else
			if (rotateFlag==2) /* CounterClockwise */
				{
					Player.dir++;
					Player.dir %=24;
				}	
			}			  
		  if (thrustFlag && (SDL_GetTicks()-speedTimer > 40)) {
			speedTimer=SDL_GetTicks();
			Player.vx += (thrustx[Player.dir]/3.0f);
			Player.vy += (thrusty[Player.dir]/3.0f);			  
		  }
		  if (hyperJump && SDL_GetTicks()-jumpTimer > 3000) {
			doHyperJump();
			hyperJump=0;
		  }
		  if (fireFlag && SDL_GetTicks()-fireTimer > 100) {
			fireTimer=SDL_GetTicks();
			doFireBullet();
		   }
		  if (paused==0) 
			MoveAsteroids();
		  RenderScreen();
		  if (paused==0) {
			MoveBullets();
			MovePlayer();
		  }
		  CycleExplosions();

		  while ( SDL_GetTicks()- tickcount < 17); // delay it to ~60 fps

	   } /* while (GameRunning) */
	exitgame:
	return;
}

#ifdef main
#undef main
#endif
int main(int argc, char *argv[]) {
	  InitSetup();
	  if (errorCount) {
		  return -1;
	  }
	SetCaption();
	InitGame();
	rotTimer = SDL_GetTicks();
	jumpTimer = SDL_GetTicks();
	GameLoop();	  
	blitAll( textures[PLBACKDROP],0 );
	print(40,250,gameovermsg);
	SDL_RenderPresent(sdlRenderer);
	SDL_Delay(2000);
	FinishOff();
	return 0;
}