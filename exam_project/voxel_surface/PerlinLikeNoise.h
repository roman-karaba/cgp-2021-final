#ifndef PERLINLIKENOISE_H
#define PERLINLIKENOISE_H

#include <vector>
#include <random>

class PerlinLikeNoise {

public:
    int size = 256;
    std::vector<float> seedVector1D;
    std::vector<float> seedVector2D;

    std::vector<float> *getSeedVector() { return &seedVector1D; }

    PerlinLikeNoise(int _size = 256)
    {
        size = _size;

        std::random_device rd; // obtain a random number from hardware
        std::mt19937 gen(rd()); // seed
        std::uniform_real_distribution<> distr(0, 1);

        // the other approach to generating random floats between 0-1 is: val = (float)rand() / (float)RAND_MAX;
        for (int i = 0; i < size; i++) seedVector1D.push_back((float)distr(gen));
        for (int i = 0; i < size*size; i++) seedVector2D.push_back((float)distr(gen));
    }

    void reseed()
    {
        std::random_device rd; // obtain a random number from hardware
        std::mt19937 gen(rd()); // seed
        std::uniform_real_distribution<> distr(0, 1);

        seedVector1D.clear();
        seedVector2D.clear();

        for (int i = 0; i < size; i++) seedVector1D.push_back((float)distr(gen));
        for (int i = 0; i < size*size; i++) seedVector2D.push_back((float)distr(gen));
    }

    void print1DSeed()
    {
        std::cout<<"1d Perlin"<<std::endl;
        for (auto element : seedVector1D) std::cout<<element<<std::endl;
    }

    void print2DSeed()
    {
        std::cout<<"2d Perlin"<<std::endl;
        for (auto element : seedVector2D) std::cout<<element<<std::endl;
    }

    std::vector<float> Noise1D(int count, std::vector<float> *seed, int numOfOctaves, float bias)
    {
        std::vector<float> outputVector(size,0.f);

        for (int noiseIndex = 0; noiseIndex < count; noiseIndex++)
        {
            float noiseAccumulator = 0.0f;
            float scaleAccumulator = 0.f;
            float samplingScale = 1.f;
            for (int octaveIndex = 0; octaveIndex < numOfOctaves; octaveIndex++)
            {
                int pitch = count >> octaveIndex; // divide by 2 cause binary shift
                int sample1 = (noiseIndex / pitch) * pitch;
                int sample2 = (sample1 + pitch) % count;

                float blend = (float)(noiseIndex - sample1) / (float)pitch;
                float sample = (1.0f - blend) * seedVector1D[sample1] + blend * seedVector1D[sample2];
                noiseAccumulator += sample * samplingScale;

                scaleAccumulator += samplingScale;
                samplingScale = samplingScale / bias;
            }

            outputVector[noiseIndex] = (noiseAccumulator / scaleAccumulator);
        }
        return outputVector;
    }

    std::vector<float> Noise2D(int width, int height, std::vector<float> *seed, int numOfOctaves, float bias)
    {
        std::vector<float> outputVector(width*height, 0.f);

        for (int noiseIndexX = 0; noiseIndexX < width; noiseIndexX++)
        {
            for (int noiseIndexY = 0; noiseIndexY < height; noiseIndexY++) {
                float noiseAccumulator = 0.0f;
                float scaleAccumulator = 0.f;
                float samplingScale = 1.f;

                for (int octaveIndex = 0; octaveIndex < numOfOctaves; octaveIndex++) {
                    int pitch = width >> octaveIndex; // divide by 2 cause binary shift
                    int sample1X = (noiseIndexX / pitch) * pitch;
                    int sample1Y = (noiseIndexY / pitch) * pitch;

                    int sample2X = (sample1X + pitch) % width;
                    int sample2Y = (sample1Y + pitch) % width;

                    float blendX = (float) (noiseIndexX - sample1X) / (float) pitch;
                    float blendY = (float) (noiseIndexY - sample1Y) / (float) pitch;

                    float sample1 = (1.0f - blendX) * seedVector2D[sample1Y * width + sample1X] + blendX * seedVector2D[sample1Y * width + sample2X];
                    float sample2 = (1.0f - blendX) * seedVector2D[sample2Y * width + sample1X] + blendX * seedVector2D[sample2Y * width + sample2X];

                    noiseAccumulator += (blendY * (sample2 - sample1) + sample1) * samplingScale;

                    scaleAccumulator += samplingScale;
                    samplingScale = samplingScale / bias;
                }

                outputVector[noiseIndexY * width + noiseIndexX] = noiseAccumulator / scaleAccumulator;
            }
        }
        return outputVector;
    }


};


#endif //PERLINLIKENOISE_H
