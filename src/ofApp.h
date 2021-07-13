#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxGui.h"
#include "ofxXmlSettings.h"
#include "ofxSyphon.h"
#include "ofxFFTouch.h"
#include "ofxTskokmtMath.h"
#include "ofxTskokmtTranslator.h"
#include "ofxTskokmtTool.h"
#include "ofxTskokmtMotion.h"

class ofApp: public ofBaseApp {

public:
    void setup();
    void update();
    void draw();
    void exit();
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
		
private:
    //mode
    int mode = 0;
    
    //time
    float time;
    float previousTime;
    
    //frame
    ofRectangle frameC;
    ofRectangle frameL;
    ofRectangle frameR;
    ofRectangle frame;
    
    //fbo
    ofFbo fbo;
    
    //osc
    ofxOscReceiver receiver;
    vector<ofxOscSender> senders;
    vector<bool> bConnecteds;
    vector<bool> bWantTouches;
    vector<bool> bWantPersons;
    vector<float> lastSignalReceivedTimes;
    ofxOscMessage message;
    float signalTerm = .2;
    
    //calibration
    ofxFloatSlider calibrationPointX, calibrationPointY;
    int calibrationPhase = 0;
    float calibrationReadiedTime = 0;
    float calibrationWaitTerm = 5;
    ofColor flashColor;
    void setMapValues();
    
    //touch
    vector<ofPoint> handsL, handsR;
    bool bReceivedHandsL, bReceivedHandsR;
    ofPoint inputMinL, inputMaxL, outputMinL, outputMaxL, inputMinR, inputMaxR, outputMinR, outputMaxR;
    vector<ofPoint> hands;
    vector<ofPoint> previousHands;
    vector<ofxFFTouch> touches;
    vector<ofxTskokmtMath> mathes;
    ofxFloatSlider minJoinDistSeed;
    float minJoinDist;
    ofxToggle bDrawRaw;
    ofxToggle bVirtualTouch;
    ofxIntSlider additionalVirtualTouch;
    
    //silhouette
    ofxSyphonClient silhouetteClient;
    
    //person
    vector<ofPoint> people;
    vector<ofPoint> previousPeople;
    ofxToggle bVirtualPerson;
    ofxIntSlider additionalVirtualPerson;
    
    //gui
    ofxPanel gui;
    
    //xml
    ofxXmlSettings xml;
    
    //syphon
    ofxSyphonServer server;
    
    //math
    ofxTskokmtMath math;
    
    //translator
    ofxTskokmtTranslator translator;
    
    //tool
    ofxTskokmtTool tool;
    
    //motion
    ofxTskokmtMotion motion;
};
