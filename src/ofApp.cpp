#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofSetWindowPosition(ofGetScreenWidth() / 18., ofGetScreenHeight() / 18.);
    ofSetWindowShape(ofGetScreenWidth() / 18. * 16, ofGetScreenHeight() / 18. * 16);
    ofBackground(0);
    ofSetCircleResolution(128);
    
    //setup frame
    xml.load("/Users/tskokmt/Documents/PROJECT/FRAME FREE/DATA/frameData.xml");
    frameL = ofRectangle(0, 0, xml.getValue("frameLW", 1340), xml.getValue("frameLH", 460));
    frameC = ofRectangle(frameL.width, 0, xml.getValue("frameCW", 1140), xml.getValue("frameCH", 460));
    frameR = ofRectangle(frameL.width + frameC.width, 0, xml.getValue("frameRW", 1340), xml.getValue("frameRH", 460));
    frame = ofRectangle(0, 0, frameC.width + frameL.width + frameR.width, frameC.height);
    
    //setup fbo
    fbo.allocate(frame.width, frame.height, GL_RGBA, 4);
    
    //setup receiver
    receiver.setup(50100);
    
    //setup gui
    gui.setup();
    gui.add(calibrationPointX.setup("calibrationPointX", 0, 0, frameC.width));
    gui.add(calibrationPointY.setup("calibrationPointY", 0, 0, frameC.height));
    gui.add(minJoinDistSeed.setup("minJoinDistSeed", 1, 0, 2));
    gui.add(bDrawRaw.setup("bDrawRaw", true));
    gui.add(bVirtualTouch.setup("bVirtualTouch", false));
    gui.add(additionalVirtualTouch.setup("additionalVirtualTouch", 0, 0, 5));
    gui.add(bVirtualPerson.setup("bVirtualPerson", false));
    gui.add(additionalVirtualPerson.setup("additionalVirtualPerson", 0, 0, 5));
    gui.loadFromFile("guiSaveData.xml");
    
    //load mapValues
    xml.clear();
    xml.load("mapValuesSaveData.xml");
    inputMinL = ofPoint(xml.getValue("inputMinLX", 0), xml.getValue("inputMinLY", 0));
    inputMaxL = ofPoint(xml.getValue("inputMaxLX", fbo.getWidth()), xml.getValue("inputMaxLY", fbo.getHeight()));
    outputMinL = ofPoint(xml.getValue("outputMinLX", 0), xml.getValue("outputMinLY", 0));
    outputMaxL = ofPoint(xml.getValue("outputMaxLX", fbo.getWidth()), xml.getValue("outputMaxLY", fbo.getHeight()));
    inputMinR = ofPoint(xml.getValue("inputMinRX", 0), xml.getValue("inputMinRY", 0));
    inputMaxR = ofPoint(xml.getValue("inputMaxRX", fbo.getWidth()), xml.getValue("inputMaxRY", fbo.getHeight()));
    outputMinR = ofPoint(xml.getValue("outputMinRX", 0), xml.getValue("outputMinRY", 0));
    outputMaxR = ofPoint(xml.getValue("outputMaxRX", fbo.getWidth()), xml.getValue("outputMaxRY", fbo.getHeight()));
    xml.clear();
    xml.load("modeSaveData.xml");
    mode = xml.getValue("mode", 0);
    
    //setup syphon
    server.setName("poolServer");
    
    //setup silhouette
    silhouetteClient.setup();
    silhouetteClient.setServerName("silhouetteServer");
    
    //load windowRect
    tool.loadWindowRect();
    
    //setup motion
    motion.setTimeSource(&time);
}

//--------------------------------------------------------------
void ofApp::update() {

    ofSetWindowTitle("pool | " + ofToString(round(ofGetFrameRate())));
    
    //update time
    previousTime = time;
    time = ofGetElapsedTimef();
    
    //send signal
    if (fmod(time, signalTerm) < fmod(previousTime, signalTerm)) {
        message.clear();
        message.setAddress("signalP");
        for (int i = 0; i < senders.size(); i++) {
            senders[i].sendMessage(message);
        }
    }
    
    //receive
    //previousHands = hands;
    while (receiver.hasWaitingMessages()) {
        message.clear();
        receiver.getNextMessage(message);
        
        //receive handsL
        if (message.getAddress() == "handL") {
            handsL.clear();
            for (int i = 1; i <= message.getArgAsInt(0) * 2; i += 2) {
                handsL.push_back(ofPoint(message.getArgAsFloat(i), message.getArgAsFloat(i + 1)));
            }
            bReceivedHandsL = true;
        }
        
        //receive handsR
        if (message.getAddress() == "handR") {
            handsR.clear();
            for (int i = 1; i <= message.getArgAsInt(0) * 2; i += 2) {
                handsR.push_back(ofPoint(message.getArgAsFloat(i), message.getArgAsFloat(i + 1)));
            }
            bReceivedHandsR = true;
        }
        
        //receive people
        if (message.getAddress() == "person") {
            people.clear();
            for (int i = 1; i <= message.getArgAsInt(0) * 2; i += 2) {
                people.push_back(ofPoint(message.getArgAsFloat(i), message.getArgAsFloat(i + 1)));
            }
        }
        
        //receive connect
        if (message.getAddress() == "connect") {
            int port = message.getArgAsInt(0);
            bool bNew = true;
            for (int i = 0; i < senders.size(); i++) {
                if (senders[i].getPort() == port) bNew = false;
            }
            if (bNew) {
                senders.push_back(ofxOscSender());
                senders.back().setup("localhost", port);
                bConnecteds.push_back(false);
                lastSignalReceivedTimes.push_back(0);
                bWantTouches.push_back(message.getArgAsBool(1));
                bWantPersons.push_back(message.getArgAsBool(2));
            }
        }
        
        //receive disconnect
        if (message.getAddress() == "disconnect") {
            int port = message.getArgAsInt(0);
            int i = 0;
            while (i < senders.size()) {
                if (senders[i].getPort() == port) {
                    senders.erase(senders.begin() + i);
                    bConnecteds.erase(bConnecteds.begin() + i);
                    lastSignalReceivedTimes.erase(lastSignalReceivedTimes.begin() + i);
                    bWantTouches.erase(bWantTouches.begin() + i);
                    bWantPersons.erase(bWantPersons.begin() + i);
                }
                else i++;
            }
        }
        
        //receive signal
        if (message.getAddress() == "signal") {
            int port = message.getArgAsInt(0);
            for (int i = 0; i < senders.size(); i++) {
                if (senders[i].getPort() == port) {
                    bConnecteds[i] = true;
                    lastSignalReceivedTimes[i] = time;
                }
            }
        }
    }
    
    //virtual touch
    if (bVirtualTouch) {
        handsL.clear();
        handsR.clear();
        if (ofGetMousePressed()) {
            handsL.push_back(translator.getDisTranslatedPosition(mouseX, mouseY) - ofPoint(frameL.width, 0));
            handsR.push_back(translator.getDisTranslatedPosition(mouseX, mouseY) - ofPoint(frameL.width, 0));
        }
        for (int i = 0; i < additionalVirtualTouch; i++) {
            if ((int)(time / (10 + i)) % 3 != 0) {
                ofPoint hand = ofPoint(ofMap(sin(time * 2 * PI / 17. + i), -1, 1, 0, frameC.width), ofMap(sin(time * 2 * PI / (13. + i * 3.3) + i * .7), -1, 1, 0, frameC.height));
                handsL.push_back(hand);
                handsR.push_back(hand);
            }
        }
    }
    
    //virtual person
    if (bVirtualPerson) {
        people.clear();
        people.push_back(ofPoint(ofNormalize(mouseX, 0, ofGetWidth()), ofNormalize(mouseY, 0, ofGetHeight())));
        for (int i = 0; i < additionalVirtualPerson; i++) {
            people.push_back(ofPoint(ofNormalize(sin(time * 2 * PI / 30. + i), -1, 1), ofNormalize(sin(time * 2 * PI / (30. + i * 10.) + i * 1.3), -1, 1)));
        }
    }
    
    //update bConnecteds
    for (int i = 0; i < bConnecteds.size(); i++) {
        if (time - lastSignalReceivedTimes[i] > signalTerm * 4) bConnecteds[i] = false;
    }
    
    if (mode == 0) {
        //set calibrationReadiedTime
        if (handsL.size() == 1 || handsR.size() == 1) {
            if (calibrationReadiedTime == 0) calibrationReadiedTime = time;
        } else {
            calibrationReadiedTime = 0;
        }
        
        //set mapValues
        if (time - calibrationReadiedTime > calibrationWaitTerm && calibrationReadiedTime != 0) setMapValues();
    } else {
        //map hands
        if (bReceivedHandsL) {
            for (int i = 0; i < handsL.size(); i++) {
                handsL[i] = math.map(handsL[i], inputMinL, inputMaxL, outputMinL, outputMaxL);
            }
        }
        if (bReceivedHandsR) {
            for (int i = 0; i < handsR.size(); i++) {
                handsR[i] = math.map(handsR[i], inputMinR, inputMaxR, outputMinR, outputMaxR);
            }
        }
        bReceivedHandsL = false;
        bReceivedHandsR = false;
    
        //set hands
        hands.clear();
        for (int i = 0; i < handsL.size(); i++) {
            hands.push_back(handsL[i]);
        }
        for (int i = 0; i < handsR.size(); i++) {
            hands.push_back(handsR[i]);
        }
        
        //nearJoin
        minJoinDist = minJoinDistSeed * sqrt(frameC.getArea()) / 12.;
        if (hands.size() >= 2) {
            vector<vector<ofPoint>> handGroups;
            handGroups.push_back(vector<ofPoint>());
            handGroups.back().push_back(hands.front());
            for (int i = 1; i < hands.size(); i++) {
                bool bNear;
                for (int j = 0; j < handGroups.size(); j++) {
                    bNear = false;
                    for (int k = 0; k < handGroups[j].size(); k++) {
                        if (ofDist(hands[i], handGroups[j][k]) < minJoinDist) {
                            bNear = true;
                            break;
                        }
                    }
                    if (bNear) {
                        handGroups[j].push_back(hands[i]);
                        break;
                    }
                }
                if (!bNear) {
                    handGroups.push_back(vector<ofPoint>());
                    handGroups.back().push_back(hands[i]);
                }
            }
            hands.clear();
            for (int i = 0; i < handGroups.size(); i++) {
                hands.push_back(math.mean(handGroups[i]));
            }
        }
        
        /*//update touches
        for (int i = 0; i < touches.size(); i++) {
            if (!touches[i].bPressing) touches[i].update(time);
        }
        //erase touches
        if (hands.size() < touches.size()) {
            vector<ofxFFTouch> shelter;
            vector<int> indexes;
            for (int i = 0; i < touches.size(); i++) {
                shelter.push_back(touches[i]);
                indexes.push_back(i);
            }
            for (int i = 0; i < hands.size(); i++) {
                vector<float> dists;
                for (int j = 0; j < shelter.size(); j++) {
                    if (shelter[j].bPressing) dists.push_back(ofDist(hands[i], shelter[j].position));
                    else dists.push_back(INT_MAX);
                }
                int index = math.minIndex(dists);
                shelter.erase(shelter.begin() + index);
                indexes.erase(indexes.begin() + index);
            }
            for (int i = touches.size() - 1; i >= 0; i--) {
                for (int j = 0; j < indexes.size(); j++) {
                    if (i == indexes[j]) {
                        if (i == touches.size() - 1) {
                            touches.erase(touches.begin() + i);
                        } else if (touches[i].bPressing) {
                            touches[i].bPressing = false;
                            touches[i].update(time);
                            touches[i].bReleased = true;
                        }
                    }
                }
            }
        }
        //swapFix
        if (hands.size() > 0) {
            for (int i = 0; i < touches.size(); i++) {
                if (!touches[i].bPressing) continue;
                vector<float> dists;
                for (int j = 0; j < hands.size(); j++) {
                    dists.push_back(ofDist(touches[i].position, hands[j]));
                }
                int index = math.minIndex(dists);
                touches[i].update(hands[index], time);
                hands.erase(hands.begin() + index);
            }
        }
        //add touches
        for (int i = 0; i < hands.size(); i++) {
            touches.push_back(ofxFFTouch());
            touches.back().setup(hands[i], time);
        }*/
        
        //swapFix touches
        /*if (hands.size() > 0 && hands.size() == previousHands.size()) {
            vector<int> indexes;
            for (int i = 0; i < hands.size(); i++) {
                vector<float> dists;
                for (int j = 0; j < previousHands.size(); j++) {
                    dists.push_back(ofDist(hands[i], previousHands[j]));
                }
                indexes.push_back(math.minIndex(dists));
            }
            vector<ofPoint> shelter = hands;
            for (int i = 0; i < hands.size(); i++) {
                hands[indexes[i]] = shelter[i];
            }
        }*/
        
        //easeFilter
        /*for (int i = 0; i < touches.size(); i++) {
            touches[i].position = mathes[i].easeFilter(mathes[i].meanFilter(touches[i].position, 2), .9);
        }*/
    }
    
    //swapFix people
    if (people.size() >= 2 && people.size() == previousPeople.size()) {
        vector<int> indexes;
        for (int i = 0; i < people.size(); i++) {
            vector<float> dists;
            for (int j = 0; j < previousPeople.size(); j++) {
                dists.push_back(ofDist(people[i], previousPeople[j]));
            }
            indexes.push_back(math.minIndex(dists));
        }
        vector<ofPoint> shelter = people;
        for (int i = 0; i < people.size(); i++) {
            people[indexes[i]] = shelter[i];
        }
    }
    previousPeople = people;
    
    /*//easeFilter people
    for (int i = 0; i < min(people.size(), previousPeople.size()); i++) {
        people[i] = previousPeople[i] + (people[i] - previousPeople[i]) * ease;
    }
    previousPeople = people;*/
    
    //send hands
    /*message.clear();
    message.setAddress("hands");
    message.addIntArg(hands.size());
    for (int i = 0; i < hands.size(); i++) {
        message.addFloatArg(hands[i].x / fbo.getWidth());
        message.addFloatArg(hands[i].y / fbo.getHeight());
    }
    for (int i = 0; i < senders.size(); i++) {
        if (bWantHands[i]) senders[i].sendMessage(message);
    }*/
    
    //send hand
    message.clear();
    message.setAddress("hand");
    message.addIntArg(hands.size());
    for (int i = 0; i < hands.size(); i++) {
        message.addFloatArg(hands[i].x / frameC.width);
        message.addFloatArg(hands[i].y / frameC.height);
    }
    for (int i = 0; i < senders.size(); i++) {
        if (bWantTouches[i]) senders[i].sendMessage(message);
    }

    /*//send touch
    message.clear();
    message.setAddress("touch");
    message.addIntArg(touches.size());
    for (int i = 0; i < touches.size(); i++) {
        message.addFloatArg(touches[i].position.x / frameC.width);
        message.addFloatArg(touches[i].position.y / frameC.height);
        message.addFloatArg(touches[i].previousPosition.x / frameC.width);
        message.addFloatArg(touches[i].previousPosition.y / frameC.height);
        message.addFloatArg(touches[i].term);
        message.addBoolArg(touches[i].bPressed);
        message.addBoolArg(touches[i].bPressing);
        message.addBoolArg(touches[i].bReleased);
    }
    for (int i = 0; i < senders.size(); i++) {
        if (bWantTouches[i]) senders[i].sendMessage(message);
    }*/
    
    //send person
    message.clear();
    message.setAddress("person");
    message.addIntArg(people.size());
    for (int i = 0; i < people.size(); i++) {
        message.addFloatArg(people[i].x);
        message.addFloatArg(people[i].y);
    }
    for (int i = 0; i < senders.size(); i++) {
        if (bWantPersons[i]) senders[i].sendMessage(message);
    }
    
    //update fbo
    fbo.begin();
    ofClear(0);
    
    float radius = sqrt(frameC.getArea()) / 90.;
    if (mode == 0) {
        ofPushMatrix();
        ofTranslate(frameL.width, 0);
        
        //draw calibration
        ofColor color;
        if (handsL.size() == 1 && handsR.size() == 1) color = ofColor(255);
        else if (handsL.size() == 1) color = ofColor(255, 0, 0);
        else if (handsR.size() == 1) color = ofColor(0, 0, 255);
        else color = ofColor(127);
        ofSetColor(flashColor, ofMap(motion.plain(), 1, 0, 255, 0));
        ofDrawRectangle(0, 0, frameC.width, frameC.height);
        ofSetColor(color);
        ofDrawCircle(calibrationPointX, calibrationPointY, radius);
        
        ofPopMatrix();
    } else {
        ofPushMatrix();
        ofTranslate(frameL.width, 0);
        
        //draw silhouette
        silhouetteClient.draw(0, 0);
        
        //draw raw hand
        if (bDrawRaw) {
            ofSetColor(127);
            for (int i = 0; i < handsL.size(); i++) {
                ofDrawCircle(handsL[i], radius);
                ofPushStyle();
                ofNoFill();
                ofSetLineWidth(sqrt(fbo.getWidth() * fbo.getHeight()) / 480.);
                ofDrawCircle(handsL[i], minJoinDist);
                ofPopStyle();
            }
            for (int i = 0; i < handsR.size(); i++) {
                ofDrawCircle(handsR[i], radius);
                ofPushStyle();
                ofNoFill();
                ofSetLineWidth(sqrt(fbo.getWidth() * fbo.getHeight()) / 480.);
                ofDrawCircle(handsR[i], minJoinDist);
                ofPopStyle();
            }
        }

        //draw hand
        for (int i = 0; i < hands.size(); i++) {
            ofColor color;
            color.setHsb((int)ofMap(i, 0, 6, 0, 256) % 256, 255, 255);
            ofSetColor(color);
            ofDrawCircle(hands[i], radius);
        }
        
        ofPopMatrix();
        
        //draw person
        ofPushStyle();
        ofNoFill();
        ofSetLineWidth(sqrt(frame.getArea()) / 120.);
        for (int i = 0; i < people.size(); i++) {
            ofColor color;
            color.setHsb((int)ofMap(i, 0, 6, 0, 256) % 256, 255, 255);
            ofSetColor(color);
            float position = ofMap(people[i].y, 0, 1, frameL.width, 0);
            ofDrawLine(position, 0, position, frame.height);
            position = ofMap(people[i].x, 0, 1, frameL.width, frameL.width + frameC.width);
            ofDrawLine(position, 0, position, frame.height);
            position = ofMap(people[i].y, 0, 1, frameL.width + frameC.width, frameL.width + frameC.width + frameR.width);
            ofDrawLine(position, 0, position, frame.height);
        }
        ofPopStyle();
    }
    
    fbo.end();
    
    //publish fbo
    server.publishTexture(&fbo.getTexture());
}

//--------------------------------------------------------------
void ofApp::draw() {
    
    //create frames
    vector<ofRectangle>frames = tool.separatedRectanglesVertical(ofGetWindowRect(), senders.size());
    
    ofPushMatrix();
    translator.reset();
    translator.smartFit(ofRectangle(0, 0, fbo.getWidth(), fbo.getHeight()), ofGetWindowRect());
    
    //draw fbo
    ofSetColor(255);
    fbo.draw(0, 0);
    
    //draw frame
    ofPushStyle();
    ofNoFill();
    ofSetColor(255);
    ofDrawRectangle(frameC);
    ofDrawRectangle(frameL);
    ofDrawRectangle(frameR);
    ofDrawRectangle(frame);
    ofPopStyle();
    
    ofPopMatrix();
    
    //draw gui
    ofSetColor(255);
    gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit() {
    
    //save gui
    gui.saveToFile("guiSaveData.xml");
    
    //save windowRect
    tool.saveWindowRect();
    
    //save mapValues
    xml.clear();
    xml.setValue("inputMinLX", inputMinL.x);
    xml.setValue("inputMinLY", inputMinL.y);
    xml.setValue("inputMaxLX", inputMaxL.x);
    xml.setValue("inputMaxLY", inputMaxL.y);
    xml.setValue("outputMinLX", outputMinL.x);
    xml.setValue("outputMinLY", outputMinL.y);
    xml.setValue("outputMaxLX", outputMaxL.x);
    xml.setValue("outputMaxLY", outputMaxL.y);
    xml.setValue("inputMinRX", inputMinR.x);
    xml.setValue("inputMinRY", inputMinR.y);
    xml.setValue("inputMaxRX", inputMaxR.x);
    xml.setValue("inputMaxRY", inputMaxR.y);
    xml.setValue("outputMinRX", outputMinR.x);
    xml.setValue("outputMinRY", outputMinR.y);
    xml.setValue("outputMaxRX", outputMaxR.x);
    xml.setValue("outputMaxRY", outputMaxR.y);
    xml.save("mapValuesSaveData.xml");
    xml.clear();
    xml.setValue("mode", mode);
    xml.save("modeSaveData.xml");
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

    //set calibration
    if (key == ' ' && mode == 0) setMapValues();
    
    //switch mode
    if (key == OF_KEY_RETURN) mode = math.loop(mode + 1, 0, 2);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

//--------------------------------------------------------------
void ofApp::setMapValues() {
    
    if (calibrationPhase == 0) {
        if (handsL.size() == 1) {
            inputMinL = handsL[0];
            outputMinL = ofPoint(calibrationPointX, calibrationPointY);
        }
        if (handsR.size() == 1) {
            inputMinR = handsR[0];
            outputMinR = ofPoint(calibrationPointX, calibrationPointY);
        }
       
    } else {
        if (handsL.size() == 1) {
            inputMaxL = handsL[0];
            outputMaxL = ofPoint(calibrationPointX, calibrationPointY);
        }
        if (handsR.size() == 1) {
            inputMaxR = handsR[0];
            outputMaxR = ofPoint(calibrationPointX, calibrationPointY);
        }
    }
    calibrationPhase++;
    if (calibrationPhase >= 2) calibrationPhase = 0;
    motion.begin(5, true);
    calibrationReadiedTime = 0;
    if (handsL.size() == 1 && handsR.size() == 1) flashColor = ofColor(255);
    else if (handsL.size() == 1) flashColor = ofColor(255, 0, 0);
    else if (handsR.size() == 1) flashColor = ofColor(0, 0, 255);
}
