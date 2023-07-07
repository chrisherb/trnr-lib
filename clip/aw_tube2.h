#pragma once
#include <cstdlib>
#include <stdint.h>

namespace trnr::lib::clip {
// modeled tube preamp based on tube2 by Chris Johnson
class aw_tube2 {
public:
    aw_tube2() {
        samplerate = 44100;

        A = 0.5;
        B = 0.5;
        previousSampleA = 0.0;
        previousSampleB = 0.0;
        previousSampleC = 0.0;
        previousSampleD = 0.0;
        previousSampleE = 0.0;
        previousSampleF = 0.0;
        fpdL = 1.0; while (fpdL < 16386) fpdL = rand()*UINT32_MAX;
        fpdR = 1.0; while (fpdR < 16386) fpdR = rand()*UINT32_MAX;
        //this is reset: values being initialized only once. Startup values, whatever they are.
    }

    void set_input(double value) {
        A = clamp(value);
    }

    void set_tube(double value) {
        B = clamp(value);
    }

    void set_samplerate(double _samplerate) {
        samplerate = _samplerate;
    }

    void process_block(double **inputs, double **outputs, long sampleframes) {
        double* in1  =  inputs[0];
        double* in2  =  inputs[1];
        double* out1 = outputs[0];
        double* out2 = outputs[1];

        double overallscale = 1.0;
        overallscale /= 44100.0;
        overallscale *= samplerate;
        
        double inputPad = A;
        double iterations = 1.0-B;
        int powerfactor = (9.0*iterations)+1;
        double asymPad = (double)powerfactor;
        double gainscaling = 1.0/(double)(powerfactor+1);
        double outputscaling = 1.0 + (1.0/(double)(powerfactor));
        
        while (--sampleframes >= 0)
        {
            double inputSampleL = *in1;
            double inputSampleR = *in2;
            if (fabs(inputSampleL)<1.18e-23) inputSampleL = fpdL * 1.18e-17;
            if (fabs(inputSampleR)<1.18e-23) inputSampleR = fpdR * 1.18e-17;
            
            if (inputPad < 1.0) {
                inputSampleL *= inputPad;
                inputSampleR *= inputPad;
            }
            
            if (overallscale > 1.9) {
                double stored = inputSampleL;
                inputSampleL += previousSampleA; previousSampleA = stored; inputSampleL *= 0.5;
                stored = inputSampleR;
                inputSampleR += previousSampleB; previousSampleB = stored; inputSampleR *= 0.5;
            } //for high sample rates on this plugin we are going to do a simple average		
            
            if (inputSampleL > 1.0) inputSampleL = 1.0;
            if (inputSampleL < -1.0) inputSampleL = -1.0;
            if (inputSampleR > 1.0) inputSampleR = 1.0;
            if (inputSampleR < -1.0) inputSampleR = -1.0;
            
            //flatten bottom, point top of sine waveshaper L
            inputSampleL /= asymPad;
            double sharpen = -inputSampleL;
            if (sharpen > 0.0) sharpen = 1.0+sqrt(sharpen);
            else sharpen = 1.0-sqrt(-sharpen);
            inputSampleL -= inputSampleL*fabs(inputSampleL)*sharpen*0.25;
            //this will take input from exactly -1.0 to 1.0 max
            inputSampleL *= asymPad;
            //flatten bottom, point top of sine waveshaper R
            inputSampleR /= asymPad;
            sharpen = -inputSampleR;
            if (sharpen > 0.0) sharpen = 1.0+sqrt(sharpen);
            else sharpen = 1.0-sqrt(-sharpen);
            inputSampleR -= inputSampleR*fabs(inputSampleR)*sharpen*0.25;
            //this will take input from exactly -1.0 to 1.0 max
            inputSampleR *= asymPad;
            //end first asym section: later boosting can mitigate the extreme
            //softclipping of one side of the wave
            //and we are asym clipping more when Tube is cranked, to compensate
            
            //original Tube algorithm: powerfactor widens the more linear region of the wave
            double factor = inputSampleL; //Left channel
            for (int x = 0; x < powerfactor; x++) factor *= inputSampleL;
            if ((powerfactor % 2 == 1) && (inputSampleL != 0.0)) factor = (factor/inputSampleL)*fabs(inputSampleL);
            factor *= gainscaling;
            inputSampleL -= factor;
            inputSampleL *= outputscaling;
            factor = inputSampleR; //Right channel
            for (int x = 0; x < powerfactor; x++) factor *= inputSampleR;
            if ((powerfactor % 2 == 1) && (inputSampleR != 0.0)) factor = (factor/inputSampleR)*fabs(inputSampleR);
            factor *= gainscaling;
            inputSampleR -= factor;
            inputSampleR *= outputscaling;
            
            if (overallscale > 1.9) {
                double stored = inputSampleL;
                inputSampleL += previousSampleC; previousSampleC = stored; inputSampleL *= 0.5;
                stored = inputSampleR;
                inputSampleR += previousSampleD; previousSampleD = stored; inputSampleR *= 0.5;
            } //for high sample rates on this plugin we are going to do a simple average
            //end original Tube. Now we have a boosted fat sound peaking at 0dB exactly
            
            //hysteresis and spiky fuzz L
            double slew = previousSampleE - inputSampleL;
            if (overallscale > 1.9) {
                double stored = inputSampleL;
                inputSampleL += previousSampleE; previousSampleE = stored; inputSampleL *= 0.5;
            } else previousSampleE = inputSampleL; //for this, need previousSampleC always
            if (slew > 0.0) slew = 1.0+(sqrt(slew)*0.5);
            else slew = 1.0-(sqrt(-slew)*0.5);
            inputSampleL -= inputSampleL*fabs(inputSampleL)*slew*gainscaling;
            //reusing gainscaling that's part of another algorithm
            if (inputSampleL > 0.52) inputSampleL = 0.52;
            if (inputSampleL < -0.52) inputSampleL = -0.52;
            inputSampleL *= 1.923076923076923;
            //hysteresis and spiky fuzz R
            slew = previousSampleF - inputSampleR;
            if (overallscale > 1.9) {
                double stored = inputSampleR;
                inputSampleR += previousSampleF; previousSampleF = stored; inputSampleR *= 0.5;
            } else previousSampleF = inputSampleR; //for this, need previousSampleC always
            if (slew > 0.0) slew = 1.0+(sqrt(slew)*0.5);
            else slew = 1.0-(sqrt(-slew)*0.5);
            inputSampleR -= inputSampleR*fabs(inputSampleR)*slew*gainscaling;
            //reusing gainscaling that's part of another algorithm
            if (inputSampleR > 0.52) inputSampleR = 0.52;
            if (inputSampleR < -0.52) inputSampleR = -0.52;
            inputSampleR *= 1.923076923076923;
            //end hysteresis and spiky fuzz section
            
            //begin 64 bit stereo floating point dither
            //int expon; frexp((double)inputSampleL, &expon);
            fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
            //inputSampleL += ((double(fpdL)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
            //frexp((double)inputSampleR, &expon);
            fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
            //inputSampleR += ((double(fpdR)-uint32_t(0x7fffffff)) * 1.1e-44l * pow(2,expon+62));
            //end 64 bit stereo floating point dither
            
            *out1 = inputSampleL;
            *out2 = inputSampleR;

            in1++;
            in2++;
            out1++;
            out2++;
        }
    }

private:
    double samplerate;

    double previousSampleA;
	double previousSampleB;
	double previousSampleC;
	double previousSampleD;
	double previousSampleE;
	double previousSampleF;
	
	uint32_t fpdL;
	uint32_t fpdR;
	//default stuff

    float A;
    float B;

    double clamp(double& value) {
        if (value > 1) {
            value = 1;
        } else if (value < 0) {
            value = 0;
        }
        return value;
    }
};
}