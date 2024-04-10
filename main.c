/*
    James William Fletcher (github.com/mrbid)
        December 2021

    Info:
    
        Space Miner, a simple 3D space mining game.


    Keyboard:

        F = FPS to console
        P = Player stats to console
        N = New Game
        Q = Break Asteroid
        E = Stop all nearby Asteroids
        R = Repel all nearby Asteroids
        W = Thrust Forward
        A = Turn Left
        S = Thrust Backward
        D = Turn Right
        Shift = Thrust Down
        Space = Thrust Up

    
    Mouse:

        Left Click = Break Asteroid
        Right Click = Repel Asteroid
        Mouse 4 Click = Stop all Asteroids nearby
        Scroll = Zoom in/out


    Notes:

        Frustum Culling:

            In a game like this by introducing frustum culling all you are doing is introducing
            frame rate instability, the fewer asteroids on-screen with frustum culling the more frames
            per second the game will get, and vice-versa. But in a game like this, you could get all
            asteroids in the frame at once, or even just very varied amounts. Frame rate instability is
            always worse than having a stable, but lower, frame rate.

            I've never been a great fan of frustum culling, mainly because it only makes sense in a
            very niche avenue of 3D games, albeit the most popular niche, FPS games.

            The popularity of First-Person can hamper creativity, other 3D camera systems are rarely
            ever explored and partly this is due to there only being four main 3D camera systems;
            Fixed Camera, Ghost/Chase Camera, Orbit Camera, and First Person Camera.

*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define uint GLushort
#define sint GLshort
#define f32 GLfloat

#include "gl.h"
#define GLFW_INCLUDE_NONE
#include "glfw3.h"

#include "esAux4.h"

#include "res.h"
#include "assets/rock1.h"
#include "assets/rock2.h"
#include "assets/rock3.h"
#include "assets/rock4.h"
#include "assets/rock5.h"
#include "assets/rock6.h"
#include "assets/rock7.h"
#include "assets/rock8.h"
#include "assets/rock9.h"
#include "assets/face.h"
#include "assets/body.h"
#include "assets/arms.h"
#include "assets/left_flame.h"
#include "assets/right_flame.h"
#include "assets/legs.h"
#include "assets/fuel.h"
#include "assets/shield.h"
#include "assets/pbreak.h"
#include "assets/pshield.h"
#include "assets/pslow.h"
#include "assets/prepel.h"

//*************************************
// globals
//*************************************
GLFWwindow* window;
uint winw = 1024;
uint winh = 768;
double t = 0;   // time
f32 dt = 0;     // delta time
double fc = 0;  // frame count
double lfct = 0;// last frame count time
f32 aspect;
double x,y,lx,ly;
double rww, ww, rwh, wh, ww2, wh2;
double uw, uh, uw2, uh2; // normalised pixel dpi

// render state id's
GLint projection_id;
GLint modelview_id;
GLint position_id;
GLint lightpos_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;

// render state inputs
vec lightpos = {0.f, 0.f, 0.f};

// models
sint bindstate = -1;
sint bindstate2 = -1;
uint keystate[6] = {0};
ESModel mdlRock[9];
ESModel mdlFace;
ESModel mdlBody;
ESModel mdlArms;
ESModel mdlLeftFlame;
ESModel mdlRightFlame;
ESModel mdlLegs;
ESModel mdlFuel;
ESModel mdlShield;
ESModel mdlPbreak;
ESModel mdlPshield;
ESModel mdlPslow;
ESModel mdlPrepel;

// game vars
#define NEWGAME_SEED 1337
#define THRUST_POWER 0.09f
#define NECK_ANGLE 0.6f
#define ROCK_DARKNESS 0.412f
#define MAX_ROCK_SCALE 12.f
const f32 RECIP_MAX_ROCK_SCALE = 1.f/(MAX_ROCK_SCALE+10.f);
#define FUEL_DRAIN_RATE 0.03f
#define SHIELD_DRAIN_RATE 0.06f
#define REFINARY_YEILD 0.13f

#ifdef __arm__
    #define ARRAY_MAX 2048 // 8 Megabytes of Asteroids
    const f32 FAR_DISTANCE = (float)ARRAY_MAX / 2.f;
#else
    #define ARRAY_MAX 16384 // 64 Megabytes of Asteroids
    f32 FAR_DISTANCE = (float)ARRAY_MAX / 2.f;
#endif
typedef struct
{
    // since we have the room
    int free; // fast free checking
    int nores;// no mineral resources

    // rock vars
    f32 scale;
    vec pos;
    vec vel;

    // +6 bytes
    uint rnd;
    f32 rndf;

    // rock colour array
    f32 colors[720];
    
    // mineral amounts
    f32 qshield;
    f32 qbreak;
    f32 qslow;
    f32 qrepel;
    f32 qfuel;

} gi; // 4+4+4+16+16+2+4+2880+4+4+4+4+4 = 2950 bytes = 4096 padded (4 kilobyte)
gi array_rocks[ARRAY_MAX] = {0};

// gets a free/unused rock
/*
// the original idea was to dynamically pop out ores when rocks are mined
// which then you need to manually float to and collect but, I went off the idea.
sint freeRock() 
{
    for(sint i = 0; i < ARRAY_MAX; i++)
        if(array_rocks[i].free == 1)
            return i;
    return -1;
}
*/

// camera vars
uint focus_cursor = 0;
double sens = 0.003f;
f32 xrot = 0.f;
f32 yrot = 0.f;
f32 zoom = -25.f;

// player vars
f32 so; // shield on (closest distance)
uint ct;// thrust signal
f32 pr; // rotation
vec pp; // position
vec pv; // velocity
vec pd; // thust direction
f32 lgr = 0.f; // last good head rotation
vec pld;// look direction
vec pfd;// face direction
f32 pf; // fuel
f32 pb; // break
f32 ps; // shield
f32 psp;// speed
f32 psl;// slow
f32 pre;// repel
uint lf;// last fuel
uint pm;// mined asteroid count
double st=0; // start time
char tts[32];// time taken string

//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}

static inline f32 fzero(f32 f)
{
    if(f < 0.f){f = 0.f;}
    return f;
}

static inline f32 fone(f32 f)
{
    if(f > 1.f){f = 1.f;}
    return f;
}

static inline f32 fsat(f32 f)
{
    if(f < 0.f){f = 0.f;}
    if(f > 1.f){f = 1.f;}
    return f;
}

void timeTaken(uint ss)
{
    if(ss == 1)
    {
        const double tt = t-st;
        if(tt < 60.0)
            sprintf(tts, "%.2f Sec", tt);
        else if(tt < 3600.0)
            sprintf(tts, "%.2f Min", tt * 0.016666667);
        else if(tt < 216000.0)
            sprintf(tts, "%.2f Hr", tt * 0.000277778);
        else if(tt < 12960000.0)
            sprintf(tts, "%.2f Days", tt * 0.00000463);
    }
    else
    {
        const double tt = t-st;
        if(tt < 60.0)
            sprintf(tts, "%.2f Seconds", tt);
        else if(tt < 3600.0)
            sprintf(tts, "%.2f Minutes", tt * 0.016666667);
        else if(tt < 216000.0)
            sprintf(tts, "%.2f Hours", tt * 0.000277778);
        else if(tt < 12960000.0)
            sprintf(tts, "%.2f Days", tt * 0.00000463);
    }
}

//*************************************
// render functions
//*************************************
void rRock(uint i, f32 dist)
{
    static const uint rcs = ARRAY_MAX / 9;
    static const f32 rrcs = 1.f / (f32)rcs;

    mIdent(&model);
    mTranslate(&model, array_rocks[i].pos.x, array_rocks[i].pos.y, array_rocks[i].pos.z);

    if(array_rocks[i].rnd < 500)
    {
        f32 mag = vMag(array_rocks[i].vel)*array_rocks[i].rndf*t;
        if(array_rocks[i].rnd < 100)
            mRotY(&model, mag);
        if(array_rocks[i].rnd < 200)
            mRotZ(&model, mag);
        if(array_rocks[i].rnd < 300)
            mRotX(&model, mag);
    }

    if(array_rocks[i].free == 2)
    {
        array_rocks[i].scale -= 32.f*dt;
        if(array_rocks[i].scale <= 0.f)
            array_rocks[i].free = 1;
        mScale(&model, array_rocks[i].scale, array_rocks[i].scale, array_rocks[i].scale);
    }
    else
        mScale(&model, array_rocks[i].scale, array_rocks[i].scale, array_rocks[i].scale);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);

    // unique colour arrays for each rock within visible distance
    if(array_rocks[i].nores == 0 && dist < 333.f)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlRock[0].cid);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rock1_colors), array_rocks[i].colors, GL_STATIC_DRAW);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);
        bindstate2 = 0;
    }
    else
    {
        if(bindstate2 != 1)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mdlRock[1].cid);
            glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(color_id);
            bindstate2 = 1;
        }
    }

    // this is a super efficient way to render 9 different types of asteroid
    uint nbs = i * rrcs;
    if(nbs > 8){nbs = 8;}
    if(nbs != bindstate)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mdlRock[nbs].vid);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);

        glBindBuffer(GL_ARRAY_BUFFER, mdlRock[nbs].nid);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlRock[nbs].iid);
        bindstate = nbs;
    }

    glDrawElements(GL_TRIANGLES, rock1_numind, GL_UNSIGNED_SHORT, 0);
}

void rLegs(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);
    f32 mag = psp*32.f;
    if(mag > 0.4f)
        mag = 0.4f;
    mRotY(&model, mag);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlLegs.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlLegs.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlLegs.iid);

    glDrawElements(GL_TRIANGLES, legs_numind, GL_UNSIGNED_SHORT, 0);
}

void rBody(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    // returns new player direction
    mGetDirZ(&pld, model);
    vInv(&pld);
    if(ct > 0)
    {
        mGetDirZ(&pd, model);
        vInv(&pd);
        ct = 0;
    }

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlBody.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlBody.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlBody.iid);

    glDrawElements(GL_TRIANGLES, body_numind, GL_UNSIGNED_SHORT, 0);
}

void rFuel(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, fone(0.062f+(1.f-pf)), fone(1.f+(1.f-pf)), fone(0.873f+(1.f-pf)));

    glBindBuffer(GL_ARRAY_BUFFER, mdlFuel.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlFuel.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlFuel.iid);

    glDrawElements(GL_TRIANGLES, fuel_numind, GL_UNSIGNED_SHORT, 0);
}

void rArms(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    f32 mag = psp*32.f;
    if(mag > 0.4f)
        mag = 0.4f;
    mRotY(&model, mag);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlArms.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlArms.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlArms.iid);

    glDrawElements(GL_TRIANGLES, arms_numind, GL_UNSIGNED_SHORT, 0);
}

void rLeftFlame(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    f32 mag = psp*32.f;
    if(mag > 0.4f)
        mag = 0.4f;
    mRotY(&model, mag);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    //glUniform3f(color_id, 1.f, 0.f, 0.f);
    glUniform3f(color_id, 0.062f, 1.f, 0.873f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlLeftFlame.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlLeftFlame.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlLeftFlame.iid);

    glDrawElements(GL_TRIANGLES, left_flame_numind, GL_UNSIGNED_SHORT, 0);
}

void rRightFlame(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    f32 mag = psp*32.f;
    if(mag > 0.4f)
        mag = 0.4f;
    mRotY(&model, mag);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 0.062f, 1.f, 0.873f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlRightFlame.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlRightFlame.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlRightFlame.iid);

    glDrawElements(GL_TRIANGLES, right_flame_numind, GL_UNSIGNED_SHORT, 0);
}

void rFace(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -xrot);

    mGetDirZ(&pfd, model);
    vInv(&pfd);
    const f32 dot = vDot(pfd, pld);
    if(dot < NECK_ANGLE)
    {
        mIdent(&model);
        mTranslate(&model, x, y, z);
        mRotX(&model, -lgr);
    }
    else
    {
        lgr = xrot;
    }

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlFace.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlFace.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlFace.iid);

    glDrawElements(GL_TRIANGLES, face_numind, GL_UNSIGNED_SHORT, 0);
}

void rBreak(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -xrot);

    vec dir;
    mGetDirZ(&dir, model);
    vInv(&dir);
    const f32 dot = vDot(dir, pld);
    if(dot < NECK_ANGLE)
    {
        mIdent(&model);
        mTranslate(&model, x, y, z);
        mRotX(&model, -lgr);
    }
    else
    {
        lgr = xrot;
    }

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, fone(0.644f+(1.f-pb)), fone(0.209f+(1.f-pb)), fone(0.f+(1.f-pb)));

    glBindBuffer(GL_ARRAY_BUFFER, mdlPbreak.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPbreak.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPbreak.iid);

    glDrawElements(GL_TRIANGLES, pbreak_numind, GL_UNSIGNED_SHORT, 0);
}

void rShield(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -xrot);

    vec dir;
    mGetDirZ(&dir, model);
    vInv(&dir);
    const f32 dot = vDot(dir, pld);
    if(dot < NECK_ANGLE)
    {
        mIdent(&model);
        mTranslate(&model, x, y, z);
        mRotX(&model, -lgr);
    }
    else
    {
        lgr = xrot;
    }

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, fone(0.f+(1.f-ps)), fone(0.8f+(1.f-ps)), fone(0.28f+(1.f-ps)));

    glBindBuffer(GL_ARRAY_BUFFER, mdlPshield.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPshield.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPshield.iid);

    glDrawElements(GL_TRIANGLES, pshield_numind, GL_UNSIGNED_SHORT, 0);
}

void rSlow(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -xrot);

    vec dir;
    mGetDirZ(&dir, model);
    vInv(&dir);
    const f32 dot = vDot(dir, pld);
    if(dot < NECK_ANGLE)
    {
        mIdent(&model);
        mTranslate(&model, x, y, z);
        mRotX(&model, -lgr);
    }
    else
    {
        lgr = xrot;
    }

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, fone(0.429f+(1.f-psl)), fone(0.f+(1.f-psl)), fone(0.8f+(1.f-psl)));

    glBindBuffer(GL_ARRAY_BUFFER, mdlPslow.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPslow.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPslow.iid);

    glDrawElements(GL_TRIANGLES, pslow_numind, GL_UNSIGNED_SHORT, 0);
}

void rRepel(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -xrot);

    vec dir;
    mGetDirZ(&dir, model);
    vInv(&dir);
    const f32 dot = vDot(dir, pld);
    if(dot < NECK_ANGLE)
    {
        mIdent(&model);
        mTranslate(&model, x, y, z);
        mRotX(&model, -lgr);
    }
    else
    {
        lgr = xrot;
    }

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, fone(0.095f+(1.f-pre)), fone(0.069f+(1.f-pre)), fone(0.041f+(1.f-pre)));

    glBindBuffer(GL_ARRAY_BUFFER, mdlPrepel.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPrepel.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPrepel.iid);

    glDrawElements(GL_TRIANGLES, prepel_numind, GL_UNSIGNED_SHORT, 0);
}

void rShieldElipse(f32 x, f32 y, f32 z, f32 rx, f32 opacity)
{
    bindstate = -1;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*)&modelview.m[0][0]);
    glUniform1f(opacity_id, opacity);
    glUniform3f(color_id, 0.f, 0.717, 0.8f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlShield.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlShield.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlShield.iid);

    glEnable(GL_BLEND);
    glDrawElements(GL_TRIANGLES, shield_numind, GL_UNSIGNED_SHORT, 0);
    glDisable(GL_BLEND);
}

void rPlayer(f32 x, f32 y, f32 z, f32 rx)
{
    psp = vMag(pv);

    rLegs(x, y, z, rx);
    rBody(x, y, z, rx);
    rFuel(x, y, z, rx);
    
    rArms(x, y+2.6f, z, rx);

    uint lf=0, rf=0;
    if(keystate[0] == 1)
        rf = 1;
    if(keystate[1] == 1)
        lf = 1;
    if(keystate[2] == 1)
        rf = 1, lf = 1;
    if(keystate[3] == 1)
        rf = 1, lf = 1;
    if(keystate[4] == 1)
        rf = 1, lf = 1;
    if(keystate[5] == 1)
        rf = 1, lf = 1;

    if(lf == 1)
        rLeftFlame(x, y+2.6f, z, rx);
    if(rf == 1)
        rRightFlame(x, y+2.6f, z, rx);

    rFace(x, y+3.4f, z, rx);
    rBreak(x, y+3.4f, z, rx);
    rShield(x, y+3.4f, z, rx);
    rSlow(x, y+3.4f, z, rx);
    rRepel(x, y+3.4f, z, rx);

    if(so > 0.f)
    {
        const f32 ss = 1.f-(so*RECIP_MAX_ROCK_SCALE);
        if(ps == 0.f)
        {
            pf -= FUEL_DRAIN_RATE * ss * dt;
            pf = fzero(pf);
        }
        else
        {
            ps -= SHIELD_DRAIN_RATE * ss * dt;
            ps = fzero(ps);
            //pv = (vec){0.f,0.f,0.f};
            //vReflect(&pv, pv, pd);

            rShieldElipse(x, y+1.f, z, rx, fsat(ss));
        }
        //printf("s: %g\n", ps);
    }
}

//*************************************
// game functions
//*************************************
void newGame(unsigned int seed)
{
    srand(seed);

    char strts[16];
    timestamp(&strts[0]);
    printf("\n[%s] Game Start [%u].\n", strts, seed);
    
    glfwSetWindowTitle(window, "Space Miner");

    f32 FDGD = FAR_DISTANCE;
#ifndef __arm__
    const f32 scalar = esRandFloat(8.f, 12.f);
    FDGD = (float)ARRAY_MAX / scalar;
    //printf("Far Distance Divisor: %g\n", scalar);
#endif
    
    pp = (vec){0.f, 0.f, 0.f};
    pv = (vec){0.f, 0.f, 0.f};
    pd = (vec){0.f, 0.f, 0.f};
    pld = (vec){0.f, 0.f, 0.f};

    st = 0;

    lf = 100;

    ct = 0;
    pm = 0;
    so = 0.f;
    pr = 0.f;
    lgr = 0.f;

    pf = 1.f;
    pb = 1.f;
    ps = 1.f;
    psl = 0.f;
    pre = 0.f;
    psp = 0.f;

    for(uint i = 0; i < ARRAY_MAX; i++)
    {
        array_rocks[i].free = 0;
        array_rocks[i].scale = esRandFloat(0.1f, MAX_ROCK_SCALE);
        array_rocks[i].pos.x = esRandFloat(-FDGD, FDGD);
        array_rocks[i].pos.y = esRandFloat(-FDGD, FDGD);
        array_rocks[i].pos.z = esRandFloat(-FDGD, FDGD);

        array_rocks[i].rnd = esRand(0, 1000);
        array_rocks[i].rndf = esRandFloat(0.05f, 0.3f);

        if(esRand(0, 1000) < 500)
        {
            array_rocks[i].qshield = esRandFloat(0.f, 1.f);
            array_rocks[i].qbreak = esRandFloat(0.f, 1.f);
            array_rocks[i].qslow = esRandFloat(0.f, 1.f);
            array_rocks[i].qrepel = esRandFloat(0.f, 1.f);
            array_rocks[i].qfuel = esRandFloat(0.f, 1.f);
        }
        else
        {
            array_rocks[i].qshield = 0.f;
            array_rocks[i].qbreak = 0.f;
            array_rocks[i].qslow = 0.f;
            array_rocks[i].qrepel = 0.f;
            array_rocks[i].qfuel = 0.f;
            array_rocks[i].nores = 1;
        }

        for(uint j = 0; j < 717; j += 3)
        {
            uint set = 0;
            #define CLR_CHANCE 0.01f

            // break
            if(esRandFloat(0.f, 1.f) < array_rocks[i].qbreak*CLR_CHANCE)
            {
                array_rocks[i].colors[j] = 0.644f;
                array_rocks[i].colors[j+1] = 0.209f;
                array_rocks[i].colors[j+2] = 0.f;
                set = 1;
            }

            // shield
            if(set == 0 && esRandFloat(0.f, 1.f) < array_rocks[i].qshield*CLR_CHANCE)
            {
                array_rocks[i].colors[j] = 0.f;
                array_rocks[i].colors[j+1] = 0.8f;
                array_rocks[i].colors[j+2] = 0.28f;
                set = 1;
            }

            // slow
            if(set == 0 && esRandFloat(0.f, 1.f) < array_rocks[i].qslow*CLR_CHANCE)
            {
                array_rocks[i].colors[j] = 0.429f;
                array_rocks[i].colors[j+1] = 0.f;
                array_rocks[i].colors[j+2] = 0.8f;
                set = 1;
            }

            // repel
            if(set == 0 && esRandFloat(0.f, 1.f) < array_rocks[i].qrepel*CLR_CHANCE)
            {
                array_rocks[i].colors[j] = 0.095f;
                array_rocks[i].colors[j+1] = 0.069f;
                array_rocks[i].colors[j+2] = 0.041f;
                set = 1;
            }

            // fuel
            if(set == 0 && esRandFloat(0.f, 1.f) < array_rocks[i].qfuel*CLR_CHANCE)
            {
                array_rocks[i].colors[j] = 0.062f;
                array_rocks[i].colors[j+1] = 1.f;
                array_rocks[i].colors[j+2] = 0.873f;
                set = 1;
            }

            // else
            if(set == 0)
            {
                array_rocks[i].colors[j] = ROCK_DARKNESS;
                array_rocks[i].colors[j+1] = ROCK_DARKNESS;
                array_rocks[i].colors[j+2] = ROCK_DARKNESS;
            }
        }

        vRuv(&array_rocks[i].vel);
    }

    st = t;
}

//*************************************
// update & render
//*************************************
void main_loop()
{
//*************************************
// time delta for interpolation
//*************************************
    static double lt = 0;
    dt = t-lt;
    lt = t;

//*************************************
// keystates
//*************************************
    if(pf == 0.f) // disable thrust control on fuel empty
        memset(&keystate[0], 0x00, sizeof(uint)*6);

    if(keystate[0] == 1)
    {
        pr += 3.f * dt;
        lgr = pr;
        pf -= FUEL_DRAIN_RATE * dt;
        pf = fzero(pf);
    }

    if(keystate[1] == 1)
    {
        pr -= 3.f * dt;
        lgr = pr;
        pf -= FUEL_DRAIN_RATE * dt;
        pf = fzero(pf);
    }
    
    if(keystate[2] == 1)
    {
        ct = 1;
        pf -= FUEL_DRAIN_RATE * dt;
        pf = fzero(pf);
    }

    if(keystate[3] == 1)
    {
        ct = 2;
        pf -= FUEL_DRAIN_RATE * dt;
        pf = fzero(pf);
    }

    if(keystate[4] == 1)
    {
        pv.y -= THRUST_POWER * dt;
        pf -= FUEL_DRAIN_RATE * dt;
        pf = fzero(pf);
    }

    if(keystate[5] == 1)
    {
        pv.y += THRUST_POWER * dt;
        pf -= FUEL_DRAIN_RATE * dt;
        pf = fzero(pf);
    }

    static double ltut = 3.0;
    const uint nf = pf*100.f;
    if(nf != lf)
    {
        char strts[16];
        timestamp(&strts[0]);
        printf("[%s] Fuel: %.2f - Speed: %g\n", strts, pf, psp*100.f);
    }
    if(nf != lf || t > ltut)
    {
        timeTaken(1);
        char title[256];
        //sprintf(title, "Space Miner - Fuel %u - Mined %u - Time %s", nf, pm, tts);
        sprintf(title, "| %s | Fuel %u | Speed %.2f | Mined %u |", tts, (uint)(pf*100.f), psp*100.f, pm);
        glfwSetWindowTitle(window, title);
        lf = nf;
        ltut = t + 3.0;
    }

    // increment player direction
    if(ct > 0)
    {
        vec inc;
        if(ct == 1)
            vMulS(&inc, pd, THRUST_POWER * dt);
        else if(ct == 2)
            vMulS(&inc, pd, -THRUST_POWER * dt);
        vAdd(&pv, pv, inc);
    }
    vAdd(&pp, pp, pv);

//*************************************
// camera
//*************************************

    if(focus_cursor == 1)
    {
        glfwGetCursorPos(window, &x, &y);

        xrot += (lx-x)*sens;
        yrot += (ly-y)*sens;

        if(yrot > 0.7f)
            yrot = 0.7f;
        if(yrot < -0.7f)
            yrot = -0.7f;

        lx = x, ly = y;
    }

    mIdent(&view);
    mTranslate(&view, 0.f, -1.5f, zoom);
    mRotate(&view, yrot, 1.f, 0.f, 0.f);
    mRotate(&view, xrot, 0.f, 1.f, 0.f);
    mTranslate(&view, -pp.x, -pp.y, -pp.z);

//*************************************
// begin render
//*************************************
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//*************************************
// main render
//*************************************

    // render player
    shadeLambert1(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    rPlayer(pp.x, pp.y, pp.z, pr);

    // render asteroids
    shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*)&projection.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    so = 0.f;
    for(uint i = 0; i < ARRAY_MAX; i++)
    {
        if(array_rocks[i].free != 1)
        {
            vec inc;
            vMulS(&inc, array_rocks[i].vel, dt);
            vAdd(&array_rocks[i].pos, array_rocks[i].pos, inc);

            const f32 dist = vDist(pp, array_rocks[i].pos);
            if(dist < 10.f + array_rocks[i].scale)
                if(so == 0.f || dist < so){so = dist;}

            rRock(i, dist);
        }
    }

//*************************************
// swap buffers / display render
//*************************************
    glfwSwapBuffers(window);
}

//*************************************
// Input Handelling
//*************************************
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // control
    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_A || key == GLFW_KEY_LEFT){ keystate[0] = 1; }
        else if(key == GLFW_KEY_D || key == GLFW_KEY_RIGHT){ keystate[1] = 1; }
        else if(key == GLFW_KEY_W || key == GLFW_KEY_UP){ keystate[2] = 1; }
        //else if(key == GLFW_KEY_S){ keystate[3] = 1; }
        else if(key == GLFW_KEY_LEFT_SHIFT){ keystate[4] = 1; }
        else if(key == GLFW_KEY_SPACE){ keystate[5] = 1; }

        // new game
        else if(key == GLFW_KEY_N)
        {
            // end
            timeTaken(0);
            char strts[16];
            timestamp(&strts[0]);
            printf("[%s] Stats: Fuel %.2f - Break %.2f - Shield %.2f - Stop %.2f - Repel %.2f - Mined %u\n", strts, pf, pb, ps, psl, pre, pm);
            printf("[%s] Time-Taken: %s or %g Seconds\n", strts, tts, t-st);
            printf("[%s] Game End.\n", strts);
            
            // new
            newGame(time(0));
        }

        // stats
        else if(key == GLFW_KEY_P)
        {
            char strts[16];
            timestamp(&strts[0]);
            printf("[%s] Stats: Fuel %.2f - Break %.2f - Shield %.2f - Stop %.2f - Repel %.2f - Mined %u\n", strts, pf, pb, ps, psl, pre, pm);
        }

        // break rocks
        else if(key == GLFW_KEY_Q && pb > 0.f)
        {
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                if(array_rocks[i].free == 0)
                {
                    const f32 dist = vDist(pp, array_rocks[i].pos);
                    if(dist < 30.f + array_rocks[i].scale)
                    {
                        pb -= 0.06f;
                        pb = fzero(pb); // hack, yes user could mine beyond pb == 0.f in this loop, take it as a last chance

                        pf += array_rocks[i].qfuel * REFINARY_YEILD * 3.f;
                        pb += array_rocks[i].qbreak * REFINARY_YEILD;
                        ps += array_rocks[i].qshield * REFINARY_YEILD;
                        psl += array_rocks[i].qslow * REFINARY_YEILD;
                        pre += array_rocks[i].qrepel * REFINARY_YEILD;

                        pf = fone(pf);
                        pb = fone(pb);
                        ps = fone(ps);
                        psl = fone(psl);
                        pre = fone(pre);

                        array_rocks[i].free = 2;
                        pm++;

                        timeTaken(1);
                        char title[256];
                        //sprintf(title, "Space Miner - Fuel %u - Mined %u - Time %s", (uint)(pf*100.f), pm, tts);
                        sprintf(title, "| %s | Fuel %u | Speed %.2f | Mined %u |", tts, (uint)(pf*100.f), psp*100.f, pm);
                        glfwSetWindowTitle(window, title);

                        char strts[16];
                        timestamp(&strts[0]);
                        printf("[%s] Break %.2f - Shield %.2f - Stop %.2f - Repel %.2f\n", strts, pb, ps, psl, pre);
                        printf("[%s] Mined: %u\n", strts, pm);
                    }
                }
            }
        }

        // stop all rocks
        else if(key == GLFW_KEY_E && psl > 0.f)
        {
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                if(array_rocks[i].free == 0 && array_rocks[i].rndf != 0.f)
                {
                    const f32 dist = vDist(pp, array_rocks[i].pos);
                    if(dist < 333.f + array_rocks[i].scale)
                    {
                        psl -= 0.06f;
                        if(psl <= 0.f)
                        {
                            psl = 0.f;
                            break;
                        }
                        array_rocks[i].vel = (vec){0.f, 0.f, 0.f};
                        array_rocks[i].rndf = 0.f;

                        char strts[16];
                        timestamp(&strts[0]);
                        printf("[%s] Repel %.2f\n", strts, psl);
                    }
                }
            }
        }

        // repel rock
        else if(key == GLFW_KEY_R && pre > 0.f)
        {
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                if(array_rocks[i].free == 0)
                {
                    const f32 dist = vDist(pp, array_rocks[i].pos);
                    if(dist < 30.f + array_rocks[i].scale)
                    {
                        //vRuv(&array_rocks[i].vel);
                        pre -= 0.06f;
                        if(pre <= 0.f)
                        {
                            pre = 0.f;
                            break;
                        }
                        array_rocks[i].vel = pfd;
                        vMulS(&array_rocks[i].vel, array_rocks[i].vel, 42.f);

                        char strts[16];
                        timestamp(&strts[0]);
                        printf("[%s] Stop %.2f\n", strts, pre);
                    }
                }
            }
        }

        // toggle mouse focus
        if(key == GLFW_KEY_ESCAPE)
        {
            focus_cursor = 0;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwGetCursorPos(window, &lx, &ly);
        }
    }
    else if(action == GLFW_RELEASE)
    {
        if(key == GLFW_KEY_A || key == GLFW_KEY_LEFT){ keystate[0] = 0; }
        else if(key == GLFW_KEY_D || key == GLFW_KEY_RIGHT){ keystate[1] = 0; }
        else if(key == GLFW_KEY_W || key == GLFW_KEY_UP){ keystate[2] = 0; }
        //else if(key == GLFW_KEY_S){ keystate[3] = 0; }
        else if(key == GLFW_KEY_LEFT_SHIFT){ keystate[4] = 0; }
        else if(key == GLFW_KEY_SPACE){ keystate[5] = 0; }
    }

    // show average fps
    if(key == GLFW_KEY_F)
    {
        if(t-lfct > 2.0)
        {
            char strts[16];
            timestamp(&strts[0]);
            printf("[%s] FPS: %g\n", strts, fc/(t-lfct));
            lfct = t;
            fc = 0;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(yoffset < 0.0)
        zoom += 0.06f * zoom;
    else
        zoom -= 0.06f * zoom;
    
    if(zoom > -15.f){zoom = -15.f;}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        if(focus_cursor == 0)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window, &lx, &ly);
            focus_cursor = 1;
        }

        if(button == GLFW_MOUSE_BUTTON_LEFT && pb > 0.f)
        {
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                if(array_rocks[i].free == 0)
                {
                    const f32 dist = vDist(pp, array_rocks[i].pos);
                    if(dist < 30.f + array_rocks[i].scale)
                    {
                        pb -= 0.06f;
                        pb = fzero(pb); // hack, yes user could mine beyond pb == 0.f in this loop, take it as a last chance

                        pf += array_rocks[i].qfuel * REFINARY_YEILD * 3.f;
                        pb += array_rocks[i].qbreak * REFINARY_YEILD;
                        ps += array_rocks[i].qshield * REFINARY_YEILD;
                        psl += array_rocks[i].qslow * REFINARY_YEILD;
                        pre += array_rocks[i].qrepel * REFINARY_YEILD;

                        pf = fone(pf);
                        pb = fone(pb);
                        ps = fone(ps);
                        psl = fone(psl);
                        pre = fone(pre);

                        array_rocks[i].free = 2;
                        pm++;

                        timeTaken(1);
                        char title[256];
                        //sprintf(title, "Space Miner - Fuel %u - Mined %u - Time %s", (uint)(pf*100.f), pm, tts);
                        sprintf(title, "| %s | Fuel %u | Speed %.2f | Mined %u |", tts, (uint)(pf*100.f), psp*100.f, pm);
                        glfwSetWindowTitle(window, title);

                        char strts[16];
                        timestamp(&strts[0]);
                        printf("[%s] Break %.2f - Shield %.2f - Stop %.2f - Repel %.2f\n", strts, pb, ps, psl, pre);
                        printf("[%s] Mined: %u\n", strts, pm);
                    }
                }
            }
        }

        if(button == GLFW_MOUSE_BUTTON_RIGHT && pre > 0.f)
        {
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                if(array_rocks[i].free == 0)
                {
                    const f32 dist = vDist(pp, array_rocks[i].pos);
                    if(dist < 30.f + array_rocks[i].scale)
                    {
                        //vRuv(&array_rocks[i].vel);
                        pre -= 0.06f;
                        if(pre <= 0.f)
                        {
                            pre = 0.f;
                            break;
                        }
                        array_rocks[i].vel = pfd;
                        vMulS(&array_rocks[i].vel, array_rocks[i].vel, 42.f);

                        char strts[16];
                        timestamp(&strts[0]);
                        printf("[%s] Repel %.2f\n", strts, pre);
                    }
                }
            }
        }

        if(button == GLFW_MOUSE_BUTTON_4 && psl > 0.f)
        {
            for(uint i = 0; i < ARRAY_MAX; i++)
            {
                if(array_rocks[i].free == 0 && array_rocks[i].rndf != 0.f)
                {
                    const f32 dist = vDist(pp, array_rocks[i].pos);
                    if(dist < 333.f + array_rocks[i].scale)
                    {
                        psl -= 0.06f;
                        if(psl <= 0.f)
                        {
                            psl = 0.f;
                            break;
                        }
                        array_rocks[i].vel = (vec){0.f, 0.f, 0.f};
                        array_rocks[i].rndf = 0.f;

                        char strts[16];
                        timestamp(&strts[0]);
                        printf("[%s] Stop %.2f\n", strts, psl);
                    }
                }
            }
        }
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width;
    winh = height;

    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = winw;
    wh = winh;
    rww = 1.0/ww;
    rwh = 1.0/wh;
    ww2 = ww/2.0;
    wh2 = wh/2.0;
    uw = (double)aspect / ww;
    uh = 1.0/wh;
    uw2 = (double)aspect / ww2;
    uh2 = 1.0/wh2;

    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 1.0f, FAR_DISTANCE);
}

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
    // allow custom msaa level
    int msaa = 16;
    if(argc >= 2){msaa = atoi(argv[1]);}

    // help
    printf("----\n");
    printf("Space Miner\n");
    printf("----\n");
    printf("James William Fletcher (github.com/mrbid)\n");
    printf("----\n");
    printf("There is only one command line argument, and that is the MSAA level 0-16.\n\n");
    printf("The aim of the game, whatever you want, try and mine every asteroid if you can. Or just see how long you can survive in space.\n\n");
	printf("On the back of your head are four color cubes, these are minerals collected from mining asteroids, these colors are also present in asteroids, these minerals are converted into on of the four actions: break/mine, repel, stop, and shield (when shiled is empty the two blue fuel cylinders will be consumed as shield).\n");
    printf("----\n");
    printf("~ Keyboard Input:\n");
    printf("ESCAPE = Release mouse lock\n");
    printf("F = FPS to console\n");
    printf("P = Player stats to console\n");
    printf("N = New Game\n");
    printf("Q = Break Asteroid\n");
    printf("E = Stop all nearby Asteroids\n");
    printf("R = Repel all nearby Asteroids\n");
    printf("W = Thrust Forward\n");
    printf("A = Turn Left\n");
    printf("D = Turn Right\n");
    printf("Shift = Thrust Down\n");
    printf("Space = Thrust Up\n");
    printf("----\n");
    printf("~ Mouse Input:\n");
    printf("Left Click = Break Asteroid\n");
    printf("Right Click = Repel Asteroid\n");
    printf("Mouse 4 Click = Stop all Asteroids nearby\n");
    printf("Scroll = Zoom in/out\n");
    printf("----\n");

    // init glfw
    if(!glfwInit()){exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, msaa);
    window = glfwCreateWindow(winw, winh, "Space Miner", NULL, NULL);
    if(!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    const GLFWvidmode* desktop = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(window, (desktop->width/2)-(winw/2), (desktop->height/2)-(winh/2)); // center window on desktop
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // set icon
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});

//*************************************
// projection
//*************************************
    window_size_callback(window, winw, winh);

//*************************************
// bind vertex and index buffers
//*************************************

    // ***** BIND FACE *****
    esBind(GL_ARRAY_BUFFER, &mdlFace.vid, face_vertices, sizeof(face_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlFace.nid, face_normals, sizeof(face_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlFace.iid, face_indices, sizeof(face_indices), GL_STATIC_DRAW);

    // ***** BIND BODY *****
    esBind(GL_ARRAY_BUFFER, &mdlBody.vid, body_vertices, sizeof(body_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlBody.nid, body_normals, sizeof(body_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlBody.iid, body_indices, sizeof(body_indices), GL_STATIC_DRAW);

    // ***** BIND ARMS *****
    esBind(GL_ARRAY_BUFFER, &mdlArms.vid, arms_vertices, sizeof(arms_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlArms.nid, arms_normals, sizeof(arms_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlArms.iid, arms_indices, sizeof(arms_indices), GL_STATIC_DRAW);

    // ***** BIND LEFT FLAME *****
    esBind(GL_ARRAY_BUFFER, &mdlLeftFlame.vid, left_flame_vertices, sizeof(left_flame_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLeftFlame.nid, left_flame_normals, sizeof(left_flame_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlLeftFlame.iid, left_flame_indices, sizeof(left_flame_indices), GL_STATIC_DRAW);

    // ***** BIND RIGHT FLAME *****
    esBind(GL_ARRAY_BUFFER, &mdlRightFlame.vid, right_flame_vertices, sizeof(right_flame_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRightFlame.nid, right_flame_normals, sizeof(right_flame_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRightFlame.iid, right_flame_indices, sizeof(right_flame_indices), GL_STATIC_DRAW);

    // ***** BIND LEGS *****
    esBind(GL_ARRAY_BUFFER, &mdlLegs.vid, legs_vertices, sizeof(legs_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLegs.nid, legs_normals, sizeof(legs_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlLegs.iid, legs_indices, sizeof(legs_indices), GL_STATIC_DRAW);

    // ***** BIND FUEL *****
    esBind(GL_ARRAY_BUFFER, &mdlFuel.vid, fuel_vertices, sizeof(fuel_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlFuel.nid, fuel_normals, sizeof(fuel_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlFuel.iid, fuel_indices, sizeof(fuel_indices), GL_STATIC_DRAW);

    // ***** BIND SHIELD *****
    esBind(GL_ARRAY_BUFFER, &mdlShield.vid, shield_vertices, sizeof(shield_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlShield.nid, shield_normals, sizeof(shield_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlShield.iid, shield_indices, sizeof(shield_indices), GL_STATIC_DRAW);

    // ***** BIND P-BREAK *****
    esBind(GL_ARRAY_BUFFER, &mdlPbreak.vid, pbreak_vertices, sizeof(pbreak_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPbreak.nid, pbreak_normals, sizeof(pbreak_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlPbreak.iid, pbreak_indices, sizeof(pbreak_indices), GL_STATIC_DRAW);

    // ***** BIND P-SHIELD *****
    esBind(GL_ARRAY_BUFFER, &mdlPshield.vid, pshield_vertices, sizeof(pshield_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPshield.nid, pshield_normals, sizeof(pshield_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlPshield.iid, pshield_indices, sizeof(pshield_indices), GL_STATIC_DRAW);

    // ***** BIND P-SLOW *****
    esBind(GL_ARRAY_BUFFER, &mdlPslow.vid, pslow_vertices, sizeof(pslow_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPslow.nid, pslow_normals, sizeof(pslow_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlPslow.iid, pslow_indices, sizeof(pslow_indices), GL_STATIC_DRAW);

    // ***** BIND P-REPEL *****
    esBind(GL_ARRAY_BUFFER, &mdlPrepel.vid, prepel_vertices, sizeof(prepel_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPrepel.nid, prepel_normals, sizeof(prepel_normals), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlPrepel.iid, prepel_indices, sizeof(prepel_indices), GL_STATIC_DRAW);

    
    /// ---


    // ***** BIND ROCK1 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].vid, rock1_vertices, sizeof(rock1_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].nid, rock1_normals, sizeof(rock1_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].cid, rock1_colors, sizeof(rock1_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[0].iid, rock1_indices, sizeof(rock1_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK2 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].vid, rock2_vertices, sizeof(rock2_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].nid, rock2_normals, sizeof(rock2_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].cid, rock2_colors, sizeof(rock2_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[1].iid, rock2_indices, sizeof(rock2_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK3 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].vid, rock3_vertices, sizeof(rock3_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].nid, rock3_normals, sizeof(rock3_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].cid, rock3_colors, sizeof(rock3_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[2].iid, rock3_indices, sizeof(rock3_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK4 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].vid, rock4_vertices, sizeof(rock4_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].nid, rock4_normals, sizeof(rock4_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].cid, rock4_colors, sizeof(rock4_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[3].iid, rock4_indices, sizeof(rock4_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK5 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].vid, rock5_vertices, sizeof(rock5_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].nid, rock5_normals, sizeof(rock5_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].cid, rock5_colors, sizeof(rock5_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[4].iid, rock5_indices, sizeof(rock5_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK6 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].vid, rock6_vertices, sizeof(rock6_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].nid, rock6_normals, sizeof(rock6_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].cid, rock6_colors, sizeof(rock6_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[5].iid, rock6_indices, sizeof(rock6_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK7 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].vid, rock7_vertices, sizeof(rock7_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].nid, rock7_normals, sizeof(rock7_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].cid, rock7_colors, sizeof(rock7_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[6].iid, rock7_indices, sizeof(rock7_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK8 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].vid, rock8_vertices, sizeof(rock8_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].nid, rock8_normals, sizeof(rock8_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].cid, rock8_colors, sizeof(rock8_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[7].iid, rock8_indices, sizeof(rock8_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK9 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[8].vid, rock9_vertices, sizeof(rock9_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[8].nid, rock9_normals, sizeof(rock9_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[8].cid, rock9_colors, sizeof(rock9_colors), GL_STATIC_DRAW);
    esBind(GL_ELEMENT_ARRAY_BUFFER, &mdlRock[8].iid, rock9_indices, sizeof(rock9_indices), GL_STATIC_DRAW);

//*************************************
// compile & link shader programs
//*************************************
    makeLambert1();
    makeLambert3();

//*************************************
// configure render options
//*************************************
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.f, 0.f, 0.f, 0.f);

//*************************************
// execute update / render loop
//*************************************

    // init
    newGame(NEWGAME_SEED);

    // reset
    t = glfwGetTime();
    lfct = t;
    
    // event loop
    while(!glfwWindowShouldClose(window))
    {
        t = glfwGetTime();
        glfwPollEvents();
        main_loop();
        fc++;
    }

    // end
    timeTaken(0);
    char strts[16];
    timestamp(&strts[0]);
    printf("[%s] Stats: Fuel %.2f - Break %.2f - Shield %.2f - Stop %.2f - Repel %.2f - Mined %u\n", strts, pf, pb, ps, psl, pre, pm);
    printf("[%s] Time-Taken: %s or %g Seconds\n", strts, tts, t-st);
    printf("[%s] Game End.\n\n", strts);

    // done
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
