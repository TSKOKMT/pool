#include "ofMain.h"
#include "ofApp.h"

//--------------------------------------------------------------
int main() {
    
    ofGLWindowSettings settings;
    auto window = ofCreateWindow(settings);
    auto app = ofPtr<ofApp>(new ofApp);
    ofRunApp(window, app);
    ofRunMainLoop();
}
