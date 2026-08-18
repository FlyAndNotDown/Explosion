#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include <CImg.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Sample::Test {
    struct Params {
        std::string samplePath;
        std::string rhi;
        std::string baselinePath;
        std::string outputDirectory;
    };

    static bool ParseParams(const int argc, char* argv[], Params& outParams)
    {
        if (argc != 9) {
            return false;
        }
        for (int i = 1; i + 1 < argc; i += 2) {
            const std::string argument = argv[i];
            const std::string value = argv[i + 1];
            if (argument == "--sample") {
                outParams.samplePath = value;
            } else if (argument == "--rhi") {
                outParams.rhi = value;
            } else if (argument == "--baseline") {
                outParams.baselinePath = value;
            } else if (argument == "--output-dir") {
                outParams.outputDirectory = value;
            } else {
                return false;
            }
        }
        return !outParams.samplePath.empty() && !outParams.rhi.empty() && !outParams.baselinePath.empty() && !outParams.outputDirectory.empty();
    }

    static std::string Quote(const std::string& value)
    {
        return '"' + value + '"';
    }

    static bool RunSample(const Params& params)
    {
        const std::filesystem::path outputDirectory(params.outputDirectory);
        const auto baselinePath = outputDirectory / "baseline.png";
        const auto actualPath = outputDirectory / "actual.png";
        const auto diffPath = outputDirectory / "diff.bmp";
        std::filesystem::create_directories(outputDirectory);
        std::filesystem::remove(actualPath);
        std::filesystem::remove(diffPath);
        std::filesystem::copy_file(params.baselinePath, baselinePath, std::filesystem::copy_options::overwrite_existing);

        std::string command = Quote(params.samplePath) + " -headless -rhi " + params.rhi + " -output " + Quote(actualPath.string());
#if CI
        command += " -softwareGpu";
#endif
        std::cout << "Running: " << command << std::endl;
        return std::system(command.c_str()) == 0;
    }

    static cimg_library::CImg<unsigned char> LoadImage(const std::string& path)
    {
        int width = 0;
        int height = 0;
        int sourceChannels = 0;
        constexpr int channels = 4;
        unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &sourceChannels, channels);
        if (pixels == nullptr) {
            throw std::runtime_error("Failed to load image: " + path);
        }

        cimg_library::CImg<unsigned char> image(width, height, 1, channels);
        cimg_forXYC(image, x, y, channel) {
            image(x, y, 0, channel) = pixels[(y * width + x) * channels + channel];
        }
        stbi_image_free(pixels);
        return image;
    }

    static bool CompareImages(const Params& params)
    {
        using Image = cimg_library::CImg<unsigned char>;
        const auto outputDirectory = std::filesystem::path(params.outputDirectory);
        const Image baseline = LoadImage((outputDirectory / "baseline.png").string());
        const Image output = LoadImage((outputDirectory / "actual.png").string());
        if (!baseline.is_sameXYZC(output)) {
            std::cerr << "Image dimensions differ: baseline=" << baseline.width() << 'x' << baseline.height() << 'x' << baseline.spectrum()
                      << ", output=" << output.width() << 'x' << output.height() << 'x' << output.spectrum() << std::endl;
            return false;
        }

        constexpr double minimumPsnr = 40.0;
        const double mse = baseline.MSE(output);
        const double psnr = baseline.PSNR(output);
        std::cout << "Image comparison: MSE=" << mse << ", PSNR=" << psnr << " dB" << std::endl;
        if (psnr >= minimumPsnr) {
            return true;
        }

        const auto diffPath = outputDirectory / "diff.bmp";
        cimg_library::CImg<float> diff(baseline);
        diff -= output;
        diff.abs().normalize(0, 255).save_bmp(diffPath.string().c_str());
        std::cerr << "Image comparison failed: PSNR is below " << minimumPsnr << " dB; diff saved to " << diffPath << std::endl;
        return false;
    }
}

int main(const int argc, char* argv[])
{
    try {
        Sample::Test::Params params;
        if (!Sample::Test::ParseParams(argc, argv, params)) {
            std::cerr << "Usage: RenderingSample.Test --sample <path> --rhi <name> --baseline <path> --output-dir <path>" << std::endl;
            return 1;
        }
        if (!Sample::Test::RunSample(params)) {
            std::cerr << "Rendering sample failed" << std::endl;
            return 1;
        }
        return Sample::Test::CompareImages(params) ? 0 : 1;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
