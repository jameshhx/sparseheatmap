#include <node.h>
#include <v8.h>
#include <nan.h>
#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include <iostream>
#include "sparsescroll.h"
#include "sparsematrix.h"
#include "sparsearray.h"
#include "colorengine.h"

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;
using v8::Array;

/**
 * Compile a series of sparse arrays to a single sparse matrix
 */
void CompileCanvas(const Nan::FunctionCallbackInfo <v8::Value> &info) {

  if (info.Length() < 15) {
    Nan::ThrowTypeError("Wrong number of arguments: expected 15.");
    return;
  }

  if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsNumber() || !info[3]->IsArray() ||
      !info[4]->IsNumber() || !info[5]->IsNumber() || !info[6]->IsArray() || !info[7]->IsNumber() ||
      !info[8]->IsArray() || !info[9]->IsNumber() || !info[10]->IsNumber() || !info[11]->IsNumber() ||
      !info[12]->IsNumber() || !info[13]->IsNumber() || !info[14]->IsNumber()) {
    Nan::ThrowTypeError(
            "Wrong arguments: Expected width (Number), height (Number), layout (Number), data (Array), blob width (Number), blob height (Number), blob intensity data (Array), imageWidth (Number), colorArray (Array), DebugMode (Number, 0 == no, 1 == yes), Filter (Number, 0 = none, 1 = lowpass).");
    return;
  }

  double width = info[0]->NumberValue();
  double height = info[1]->NumberValue();
  int layout = info[2]->NumberValue();
  v8::Local <v8::Array> dataarr = v8::Local<v8::Array>::Cast(info[3]);
  double blobwidth = info[4]->NumberValue();
  double blobheight = info[5]->NumberValue();
  int destImageWidth = (int) info[7]->NumberValue();

  int trimPixelsLeft = (int) info[11]->NumberValue();
  int trimPixelsTop = (int) info[12]->NumberValue();
  int trimPixelsRight = (int) info[13]->NumberValue();
  int trimPixelsBottom = (int) info[14]->NumberValue();

  int debugMode = (int) info[9]->NumberValue();
  if (debugMode == 1) {
    std::cout << "SparseHeatmap DEBUG: Native extensions debug mode on.\n";
  }
  int filter = (int) info[10]->NumberValue();
  if (debugMode == 1) {
    std::cout << "SparseHeatmap DEBUG: Filter: " << filter << ".\n";
  }
  v8::Local <v8::Array> blobArr = v8::Local<v8::Array>::Cast(info[6]);
  v8::Local <v8::Array> colorArr = v8::Local<v8::Array>::Cast(info[8]);

  Colorengine *c = new Colorengine(round((float) colorArr->Length() / 4.0));
  if (debugMode == 1) {
    std::cout << "SparseHeatmap DEBUG: Adding " << (colorArr->Length() / 4) << " color maps.\n";
  }
  for (unsigned int p = 0; p < colorArr->Length(); p += 4) {
    c->add_color((int) colorArr->Get(p)->Uint32Value(), (int) colorArr->Get(p + 1)->Uint32Value(),
                 (int) colorArr->Get(p + 2)->Uint32Value(), (int) colorArr->Get(p + 3)->Uint32Value());
  }
  c->prepare();

  unsigned int *blobVals = new unsigned int[blobArr->Length()];
  // Iterate through the blob array, adding each element to our list
  for (unsigned int i = 0; i < blobArr->Length(); i++) {
    unsigned int intensityval = blobArr->Get(i)->Uint32Value();
    blobVals[i] = intensityval;
  }

  Sparsearray **sparrs = new Sparsearray *[dataarr->Length()];

  // Do the matrix!
  Sparsematrix matrix(width, height, blobwidth, blobheight, layout, blobVals, debugMode, filter, trimPixelsLeft, trimPixelsTop, trimPixelsRight, trimPixelsBottom);

  for (unsigned int d = 0; d < dataarr->Length(); d++) {
    Sparsearray *myNewSP = new Sparsearray();
    v8::Local <v8::Object> myObj = v8::Local<v8::Object>::Cast(dataarr->Get(d));
    myNewSP->width = (unsigned int) myObj->GetInternalField(0)->NumberValue();
    myNewSP->height = (unsigned int) myObj->GetInternalField(1)->NumberValue();
    v8::Local <v8::Array> sparseArr = v8::Local<v8::Array>::Cast(myObj->GetInternalField(2));
    myNewSP->datalen = sparseArr->Length();
    myNewSP->data = new unsigned int[sparseArr->Length()];
    for (unsigned int t = 0; t < sparseArr->Length(); t++) {
      unsigned int scoreval = sparseArr->Get(t)->Uint32Value();
      myNewSP->data[t] = scoreval;
    }
    sparrs[d] = myNewSP;

    matrix.integrate_sparsearray(myNewSP);
  }

  unsigned char *finalImageIntensities = matrix.get_intensity_map(destImageWidth, c);
/*
  for (int p = 0; p <= matrix.lastIntensityIndex; p += 4) {
    finalImageIntensities[p] = 255;
    finalImageIntensities[p + 1] = 0;
    finalImageIntensities[p + 2] = 0;
    finalImageIntensities[p + 3] = 255;
  }*/
  //Local <Array> v8Array = Nan::New<Array>();
  /*for (int p = 0; p < (256 * 4); p++) {
    v8Array->Set(p, Nan::New(c->finalcolors[p]));
  }*/
  /*for (int p = 0; p < 4 * 256; p++) {
    v8Array->Set(p, Nan::New(c->finalcolors[p]));
  }*/
  /*v8Array->Set(0, Nan::New(c->finalcolors[(255 * 4)]));
  v8Array->Set(1, Nan::New(c->finalcolors[(255 * 4) + 1]));
  v8Array->Set(2, Nan::New(c->finalcolors[(255 * 4) + 2]));
  v8Array->Set(3, Nan::New(c->finalcolors[(255 * 4)] + 3));
*/

  for (unsigned int s = 0; s < dataarr->Length(); s++) {
    delete sparrs[s];
  }
  delete[] sparrs;
  delete[] blobVals;
  delete c;

  //info.GetReturnValue().Set(Nan::NewBuffer(c->finalcolors, 4 * 256).ToLocalChecked());
  info.GetReturnValue().Set(Nan::NewBuffer((char *) finalImageIntensities, matrix.lastIntensityIndex).ToLocalChecked());
  //info.GetReturnValue().Set(v8Array);
}

/**
 * Compile a series of sparse arrays to a single sparse matrix
 */
void CompileVScroll(const Nan::FunctionCallbackInfo <v8::Value> &info) {

  if (info.Length() < 8) {
    Nan::ThrowTypeError("Wrong number of arguments: expected 8.");
    return;
  }

  if (!info[0]->IsNumber() || !info[1]->IsNumber() || !info[2]->IsArray() || !info[3]->IsNumber() ||
      !info[4]->IsNumber() || !info[5]->IsArray() || !info[6]->IsNumber() || !info[7]->IsNumber()) {
    Nan::ThrowTypeError(
            "Wrong arguments: Canvas width (Number), Canvas height (Number), data (Array), Output image width (Number), Y-axis pixel multiplier (Number), Color array (Number), Debug mode (Number, 0 == no, 1 == yes), Filter (Number, 0 = none, 1 = lowpass).");
    return;
  }

  double width = info[0]->NumberValue();
  double height = info[1]->NumberValue();
  double destImageWidth = info[3]->NumberValue();
  int debugMode = (int) info[6]->NumberValue();
  if (debugMode == 1) {
    std::cout << "SparseHeatmap DEBUG: Native extensions debug mode on.\n";
  }
  int filter = (int) info[7]->NumberValue();
  if (debugMode == 1) {
    std::cout << "SparseHeatmap DEBUG: Filter: " << filter << ".\n";
  }
  int yAxisMultiplier = (int) round((double) info[4]->NumberValue());
  if (yAxisMultiplier < 1) {
    yAxisMultiplier = 1;
  }
  v8::Local <v8::Array> dataarr = v8::Local<v8::Array>::Cast(info[2]);
  v8::Local <v8::Array> colorArr = v8::Local<v8::Array>::Cast(info[5]);

  Colorengine *c = new Colorengine(round((float) colorArr->Length() / 4.0));
  if (debugMode == 1) {
    std::cout << "SparseHeatmap DEBUG: Adding " << (colorArr->Length() / 4) << " color maps.\n";
  }
  for (unsigned int p = 0; p < colorArr->Length(); p += 4) {
    c->add_color((int) colorArr->Get(p)->Uint32Value(), (int) colorArr->Get(p + 1)->Uint32Value(),
                 (int) colorArr->Get(p + 2)->Uint32Value(), (int) colorArr->Get(p + 3)->Uint32Value());
  }
  c->prepare();

  Sparsearray **sparrs = new Sparsearray *[dataarr->Length()];

  // Do the matrix!
  Sparsescroll matrix(width, height, yAxisMultiplier, debugMode, filter);

  for (unsigned int d = 0; d < dataarr->Length(); d++) {
    Sparsearray *myNewSP = new Sparsearray();
    v8::Local <v8::Object> myObj = v8::Local<v8::Object>::Cast(dataarr->Get(d));
    myNewSP->width = (unsigned int) myObj->GetInternalField(0)->NumberValue();
    myNewSP->height = (unsigned int) myObj->GetInternalField(1)->NumberValue();
    v8::Local <v8::Array> sparseArr = v8::Local<v8::Array>::Cast(myObj->GetInternalField(2));
    myNewSP->datalen = sparseArr->Length();
    myNewSP->data = new unsigned int[sparseArr->Length()];
    for (unsigned int t = 0; t < sparseArr->Length(); t++) {
      unsigned int scoreval = sparseArr->Get(t)->Uint32Value();
      myNewSP->data[t] = scoreval;
    }
    sparrs[d] = myNewSP;

    matrix.integrate_sparsearray(myNewSP);
  }

  unsigned char *finalImageIntensities = matrix.get_intensity_map(destImageWidth, c);

  /*Local <Array> v8Array = Nan::New<Array>();
  unsigned int matlen = matrix.height;
  for (unsigned int s = 0; s < matlen; s++) {
    v8Array->Set(s, Nan::New((double) matrix.data[s]));
  }*/

  for (unsigned int s = 0; s < dataarr->Length(); s++) {
    delete sparrs[s];
  }
  delete[] sparrs;
  delete c;

  //info.GetReturnValue().Set(v8Array);
  info.GetReturnValue().Set(Nan::NewBuffer((char *) finalImageIntensities, matrix.lastIntensityIndex).ToLocalChecked());
}

/**
 * Interface with JavaScript and expose our API
 */
void Init(v8::Local <v8::Object> exports) {
  exports->Set(Nan::New("compile_canvas").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(CompileCanvas)->GetFunction());
  exports->Set(Nan::New("compile_vertical_scroll").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(CompileVScroll)->GetFunction());
}

// Let node know about this
NODE_MODULE(sparsematrix, Init
)