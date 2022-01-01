/*
    James William Fletcher (james@voxdsp.com)
        December 2021

    Info:
    
        Space Miner, a simple 3D space mining game.


    Keyboard:

        W = Thrust Forward
        A = Turn Left
        S = Thrust Backward
        D = Turn Right
        Shift = Thrust Down
        Space = Thrust Up


    Plan:

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

#ifndef __x86_64__ 
    #define NOSSE
#endif
#define SEIR_RAND
#include "esAux2.h"

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
double dt = 0;  // delta time
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
GLint solidcolor_id;
GLint color_id;
GLint opacity_id;
GLint normal_id;

// render state matrices
mat projection;
mat view;
mat model;
mat modelview;
mat viewrot;

// render state inputs
vec lightpos = {0.f, 0.f, 0.f};

// models
uint bindstate = 0;
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
#define FAR_DISTANCE 512.f
#define THRUST_POWER 0.03f
#define NECK_ANGLE 0.6f

#define ARRAY_MAX 2048 // 2 Megabytes of Asteroids
typedef struct
{
    // since we have the room
    int free; // fast free checking

    // rock vars
    f32 scale;
    f32 colors[240];
    vec pos;
    vec vel;
    char type; // rock 0-9
    
    // mineral amounts
    f32 qshield;
    f32 qbreak;
    f32 qslow;
    f32 qrepel;
    f32 qfuel;
} gi; // 4+960+32+1+20 = 1021 bytes = 1024 padded (1 kilobyte)
gi array_rocks[ARRAY_MAX];

// gets a free/unused rock
sint freeRock()
{
    for(sint i = 0; i < ARRAY_MAX; i++)
        if(array_rocks[i].free == 1)
            return i;
    return -1;
}

// camera vars
uint focus_cursor = 1;
vec cp;
double sens = 0.01f;
f32 xrot = 0.f;
f32 yrot = 0.f;
f32 zoom = -25.f;

// player vars
uint ct;// trust signal
f32 pr; // rotation
vec pp; // position
vec pv; // velocity
vec pd; // thust direction
f32 lgr = 0.f;
vec pld;// look direction


//*************************************
// utility functions
//*************************************
void timestamp(char* ts)
{
    const time_t tt = time(0);
    strftime(ts, 16, "%H:%M:%S", localtime(&tt));
}

//*************************************
// render functions
//*************************************

void rRock(f32 x, f32 y, f32 z)
{
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    
    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlRock[0].vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlRock[0].nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlRock[0].cid);
    glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(color_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlRock[0].iid);

    glDrawElements(GL_TRIANGLES, rock1_numind, GL_UNSIGNED_SHORT, 0);
}

void rLegs(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = 0;

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

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
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
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
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
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 0.062f, 1.f, 0.873f);

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
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
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
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 0.f, 0.f);

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
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 0.f, 0.f);

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
    bindstate = 0;

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

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
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
    bindstate = 0;

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

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

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
    bindstate = 0;

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

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

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
    bindstate = 0;

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

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

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
    bindstate = 0;

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

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 1.0f);
    glUniform3f(color_id, 1.f, 1.f, 1.f);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPrepel.vid);
    glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_id);

    glBindBuffer(GL_ARRAY_BUFFER, mdlPrepel.nid);
    glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normal_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mdlPrepel.iid);

    glDrawElements(GL_TRIANGLES, prepel_numind, GL_UNSIGNED_SHORT, 0);
}

void rShieldElipse(f32 x, f32 y, f32 z, f32 rx)
{
    bindstate = 0;

    mIdent(&model);
    mTranslate(&model, x, y, z);
    mRotX(&model, -rx);

    mMul(&modelview, &model, &view);

    glUniformMatrix4fv(projection_id, 1, GL_FALSE, (f32*) &projection.m[0][0]);
    glUniformMatrix4fv(modelview_id, 1, GL_FALSE, (f32*) &modelview.m[0][0]);
    glUniform3f(lightpos_id, lightpos.x, lightpos.y, lightpos.z);
    glUniform1f(opacity_id, 0.5f);
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
    rLegs(x, y, z, rx);
    rBody(x, y, z, rx);
    rFuel(x, y, z, rx);
    
    rArms(x, y+2.6f, z, rx);
    rLeftFlame(x, y+2.6f, z, rx);
    rRightFlame(x, y+2.6f, z, rx);

    rFace(x, y+3.4f, z, rx);
    rBreak(x, y+3.4f, z, rx);
    rShield(x, y+3.4f, z, rx);
    rSlow(x, y+3.4f, z, rx);
    rRepel(x, y+3.4f, z, rx);

    //rShieldElipse(x, y+1.f, z, rx);
}

//*************************************
// game functions
//*************************************
void newGame()
{
    char strts[16];
    timestamp(&strts[0]);
    printf("[%s] Game Start.\n", strts);
    
    glfwSetWindowTitle(window, "Space Miner");
    
    cp = (vec){0.f, 0.f, 0.f};
    pp = (vec){0.f, 0.f, 0.f};
    pv = (vec){0.f, 0.f, 0.f};

    //

// typedef struct
// {
//     // since we have the room
//     int free; // fast free checking

//     // rock vars
//     f32 scale;
//     f32 colors[240];
//     vec pos;
//     vec vel;
//     char type; // rock 0-9
    
//     // mineral amounts
//     f32 qshield;
//     f32 qbreak;
//     f32 qslow;
//     f32 qrepel;
//     f32 qfuel;
// } gi;

    for(uint i = 0; i < ARRAY_MAX; i++)
    {
        array_rocks[i].free = 0;
        array_rocks[i].scale = 1.f;
        array_rocks[i].pos.x = esRandFloat(-FAR_DISTANCE, FAR_DISTANCE);
        array_rocks[i].pos.y = esRandFloat(-FAR_DISTANCE, FAR_DISTANCE);
        array_rocks[i].pos.z = esRandFloat(-FAR_DISTANCE, FAR_DISTANCE);
        // array_rocks[i].pos.x = 0.f;
        // array_rocks[i].pos.y = 0.f;
        // array_rocks[i].pos.z = 0.f;
        vRuv(&array_rocks[i].vel);
    }
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
    if(keystate[0] == 1)
    {
        pr += 3.f * dt;
        lgr = pr;
    }

    if(keystate[1] == 1)
    {
        pr -= 3.f * dt;
        lgr = pr;
    }
    
    if(keystate[2] == 1)
        ct = 1;

    if(keystate[3] == 1)
        ct = 2;

    if(keystate[4] == 1)
        pv.y -= THRUST_POWER * dt;

    if(keystate[5] == 1)
        pv.y += THRUST_POWER * dt;

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

        xrot += (ww2-x)*sens;
        yrot += (wh2-y)*sens;

        if(yrot > 0.7f)
            yrot = 0.7f;
        if(yrot < -0.7f)
            yrot = -0.7f;

        glfwSetCursorPos(window, ww2, wh2);
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
    rPlayer(pp.x, pp.y, pp.z, pr);

    // render asteroids
    shadeLambert3(&position_id, &projection_id, &modelview_id, &lightpos_id, &normal_id, &color_id, &opacity_id);
    for(uint i = 0; i < ARRAY_MAX; i++)
    {
        if(array_rocks[i].free == 0)
        {
            vec inc;
            vMulS(&inc, array_rocks[i].vel, dt);
            vAdd(&array_rocks[i].pos, array_rocks[i].pos, inc);
            rRock(array_rocks[i].pos.x, array_rocks[i].pos.y, array_rocks[i].pos.z);
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
        if(key == GLFW_KEY_A){ keystate[0] = 1; }
        else if(key == GLFW_KEY_D){ keystate[1] = 1; }
        else if(key == GLFW_KEY_W){ keystate[2] = 1; }
        else if(key == GLFW_KEY_S){ keystate[3] = 1; }
        else if(key == GLFW_KEY_LEFT_SHIFT){ keystate[4] = 1; }
        else if(key == GLFW_KEY_SPACE){ keystate[5] = 1; }

        // // fire bullet
        // else if(key == GLFW_KEY_SPACE)
        // {
        //     for(uint i = 0; i < ARRAY_MAX; i++)
        //     {
        //         if(array_bullet[i].z == 0.f)
        //         {
        //             array_bullet[i].x = pp.x;
        //             array_bullet[i].y = pp.y;
        //             array_bullet[i].z = -8.f;
        //             break;
        //         }
        //     }
        // }

        // // random seed
        // else if(key == GLFW_KEY_R)
        // {
        //     const unsigned int nr = time(0);
        //     srand(t*100);
        //     srandf(t*100);
        //     char strts[16];
        //     timestamp(&strts[0]);
        //     printf("[%s] New Random Seed: %u\n", strts, nr);

        //     for(uint i = 0; i < ARRAY_MAX; i++)
        //         rndCube(i, esRandFloat(-64.f, -FAR_DISTANCE));
        // }

        // toggle mouse focus
        if(key == GLFW_KEY_ESCAPE)
        {
            focus_cursor = 1 - focus_cursor;
            if(focus_cursor == 0)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            glfwSetCursorPos(window, ww2, wh2);
            glfwGetCursorPos(window, &ww2, &wh2);
            
        }
    }
    else if(action == GLFW_RELEASE)
    {
        if(key == GLFW_KEY_A){ keystate[0] = 0; }
        else if(key == GLFW_KEY_D){ keystate[1] = 0; }
        else if(key == GLFW_KEY_W){ keystate[2] = 0; }
        else if(key == GLFW_KEY_S){ keystate[3] = 0; }
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
    if(yoffset == -1)
    {
        zoom -= 1.0f;
    }
    else if(yoffset == 1)
    {
        zoom += 1.0f;
    }
}

// void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
// {
//     if(button == GLFW_MOUSE_BUTTON_LEFT)
//     {
//         if(action == GLFW_PRESS)
//         {
//             for(uint i = 0; i < ARRAY_MAX; i++)
//             {
//                 if(array_bullet[i].z == 0.f)
//                 {
//                     array_bullet[i].x = x;
//                     array_bullet[i].y = y;
//                     array_bullet[i].z = -8.f;
//                     break;
//                 }
//             }
//         }
//     }
// }

void window_size_callback(GLFWwindow* window, int width, int height)
{
    winw = width;
    winh = height;

    glViewport(0, 0, winw, winh);
    aspect = (f32)winw / (f32)winh;
    ww = winw;
    wh = winh;
    rww = 1/ww;
    rwh = 1/wh;
    ww2 = ww/2;
    wh2 = wh/2;
    uw = (double)aspect / ww;
    uh = 1 / wh;
    uw2 = (double)aspect / ww2;
    uh2 = 1 / wh2;

    mIdent(&projection);
    mPerspective(&projection, 60.0f, aspect, 1.0f, FAR_DISTANCE); 
}

//*************************************
// Process Entry Point
//*************************************
int main(int argc, char** argv)
{
    // help
    printf("Space Miner\n");
    printf("James William Fletcher (james@voxdsp.com)\n\n");
    printf(".....\n\n");

    // init glfw
    if(!glfwInit()){exit(EXIT_FAILURE);}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 16);
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
    //glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glfwSwapInterval(1); // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync

    // set icon
    glfwSetWindowIcon(window, 1, &(GLFWimage){16, 16, (unsigned char*)&icon_image.pixel_data});

    // hide cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // seed random
    srand(NEWGAME_SEED);

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
    esBind(GL_ARRAY_BUFFER, &mdlFace.iid, face_indices, sizeof(face_indices), GL_STATIC_DRAW);

    // ***** BIND BODY *****
    esBind(GL_ARRAY_BUFFER, &mdlBody.vid, body_vertices, sizeof(body_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlBody.nid, body_normals, sizeof(body_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlBody.iid, body_indices, sizeof(body_indices), GL_STATIC_DRAW);

    // ***** BIND ARMS *****
    esBind(GL_ARRAY_BUFFER, &mdlArms.vid, arms_vertices, sizeof(arms_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlArms.nid, arms_normals, sizeof(arms_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlArms.iid, arms_indices, sizeof(arms_indices), GL_STATIC_DRAW);

    // ***** BIND LEFT FLAME *****
    esBind(GL_ARRAY_BUFFER, &mdlLeftFlame.vid, left_flame_vertices, sizeof(left_flame_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLeftFlame.nid, left_flame_normals, sizeof(left_flame_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLeftFlame.iid, left_flame_indices, sizeof(left_flame_indices), GL_STATIC_DRAW);

    // ***** BIND RIGHT FLAME *****
    esBind(GL_ARRAY_BUFFER, &mdlRightFlame.vid, right_flame_vertices, sizeof(right_flame_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRightFlame.nid, right_flame_normals, sizeof(right_flame_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRightFlame.iid, right_flame_indices, sizeof(right_flame_indices), GL_STATIC_DRAW);

    // ***** BIND LEGS *****
    esBind(GL_ARRAY_BUFFER, &mdlLegs.vid, legs_vertices, sizeof(legs_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLegs.nid, legs_normals, sizeof(legs_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlLegs.iid, legs_indices, sizeof(legs_indices), GL_STATIC_DRAW);

    // ***** BIND FUEL *****
    esBind(GL_ARRAY_BUFFER, &mdlFuel.vid, fuel_vertices, sizeof(fuel_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlFuel.nid, fuel_normals, sizeof(fuel_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlFuel.iid, fuel_indices, sizeof(fuel_indices), GL_STATIC_DRAW);

    // ***** BIND SHIELD *****
    esBind(GL_ARRAY_BUFFER, &mdlShield.vid, shield_vertices, sizeof(shield_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlShield.nid, shield_normals, sizeof(shield_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlShield.iid, shield_indices, sizeof(shield_indices), GL_STATIC_DRAW);

    // ***** BIND P-BREAK *****
    esBind(GL_ARRAY_BUFFER, &mdlPbreak.vid, pbreak_vertices, sizeof(pbreak_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPbreak.nid, pbreak_normals, sizeof(pbreak_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPbreak.iid, pbreak_indices, sizeof(pbreak_indices), GL_STATIC_DRAW);

    // ***** BIND P-SHIELD *****
    esBind(GL_ARRAY_BUFFER, &mdlPshield.vid, pshield_vertices, sizeof(pshield_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPshield.nid, pshield_normals, sizeof(pshield_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPshield.iid, pshield_indices, sizeof(pshield_indices), GL_STATIC_DRAW);

    // ***** BIND P-SLOW *****
    esBind(GL_ARRAY_BUFFER, &mdlPslow.vid, pslow_vertices, sizeof(pslow_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPslow.nid, pslow_normals, sizeof(pslow_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPslow.iid, pslow_indices, sizeof(pslow_indices), GL_STATIC_DRAW);

    // ***** BIND P-REPEL *****
    esBind(GL_ARRAY_BUFFER, &mdlPrepel.vid, prepel_vertices, sizeof(prepel_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPrepel.nid, prepel_normals, sizeof(prepel_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlPrepel.iid, prepel_indices, sizeof(prepel_indices), GL_STATIC_DRAW);

    
    /// ---


    // ***** BIND ROCK1 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].vid, rock1_vertices, sizeof(rock1_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].nid, rock1_normals, sizeof(rock1_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].cid, rock1_colors, sizeof(rock1_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[0].iid, rock1_indices, sizeof(rock1_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK2 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].vid, rock2_vertices, sizeof(rock2_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].nid, rock2_normals, sizeof(rock2_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].cid, rock2_colors, sizeof(rock2_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[1].iid, rock2_indices, sizeof(rock2_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK3 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].vid, rock3_vertices, sizeof(rock3_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].nid, rock3_normals, sizeof(rock3_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].cid, rock3_colors, sizeof(rock3_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[2].iid, rock3_indices, sizeof(rock3_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK4 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].vid, rock4_vertices, sizeof(rock4_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].nid, rock4_normals, sizeof(rock4_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].cid, rock4_colors, sizeof(rock4_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[3].iid, rock4_indices, sizeof(rock4_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK5 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].vid, rock5_vertices, sizeof(rock5_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].nid, rock5_normals, sizeof(rock5_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].cid, rock5_colors, sizeof(rock5_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[4].iid, rock5_indices, sizeof(rock5_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK6 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].vid, rock6_vertices, sizeof(rock6_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].nid, rock6_normals, sizeof(rock6_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].cid, rock6_colors, sizeof(rock6_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[5].iid, rock6_indices, sizeof(rock6_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK7 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].vid, rock7_vertices, sizeof(rock7_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].nid, rock7_normals, sizeof(rock7_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].cid, rock7_colors, sizeof(rock7_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[6].iid, rock7_indices, sizeof(rock7_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK8 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].vid, rock8_vertices, sizeof(rock8_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].nid, rock8_normals, sizeof(rock8_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].cid, rock8_colors, sizeof(rock8_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[7].iid, rock8_indices, sizeof(rock8_indices), GL_STATIC_DRAW);

    // ***** BIND ROCK9 *****
    esBind(GL_ARRAY_BUFFER, &mdlRock[8].vid, rock9_vertices, sizeof(rock9_vertices), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[8].nid, rock9_normals, sizeof(rock9_normals), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[8].cid, rock9_colors, sizeof(rock9_colors), GL_STATIC_DRAW);
    esBind(GL_ARRAY_BUFFER, &mdlRock[8].iid, rock9_indices, sizeof(rock9_indices), GL_STATIC_DRAW);

//*************************************
// compile & link shader programs
//*************************************

    //makeAllShaders();
    makeLambert1();
    makeLambert3();

//*************************************
// configure render options
//*************************************

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);

//*************************************
// execute update / render loop
//*************************************

    // init
    newGame();

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

    // final score
    // char strts[16];
    // timestamp(&strts[0]);
    // printf("[%s] SCORE %i - HIT %u - LOSS %u\n\n", strts, score, hit, loss);

    // done
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
    return 0;
}
