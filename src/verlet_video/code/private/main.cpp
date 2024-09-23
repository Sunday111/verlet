#include <fstream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "fmt/core.h"
#include "fmt/std.h"  // IWYU pragma: keep
#include "klgl/error_handling.hpp"
#include "verlet/coloring/spawn_color/spawn_color_strategy_array.hpp"
#include "verlet/gui/app_gui.hpp"
#include "verlet/verlet_app.hpp"

namespace verlet
{
static constexpr int FourCC(std::optional<std::string_view> maybe_name)
{
    if (maybe_name.has_value())
    {
        auto name = *maybe_name;
        return cv::VideoWriter::fourcc(name[0], name[1], name[2], name[3]);
    }

    return 0;
}

static constexpr int kOutputVideoFPS = 60;
static constexpr std::string_view kOutputVideoFormat = ".mp4";
static constexpr std::optional<std::string_view> kVideoEncoding = "avc1";

class VerletVideoApp : public VerletApp
{
public:
    using Super = VerletApp;

    std::vector<edt::Vec3u8> frame_pixels_;
    std::unique_ptr<cv::VideoWriter> video_writer_;
    size_t frames_written_ = 0;
    cv::Mat frame_;

    void Initialize() override
    {
        Super::Initialize();
        LoadAppState(GetExecutableDir() / AppGUI::kDefaultPresetFileName);
        const auto window_size = GetWindow().GetSize();
        const auto window_size_i = window_size.Cast<int>();

        UpdateWorldRange(std::numeric_limits<float>::max());
        const auto sim_area = solver.GetSimArea();
        const auto image_path = (GetExecutableDir() / "van.jpg");

        spawn_color_strategy_ = [&]
        {
            cv::Mat image = cv::imread(image_path, cv::IMREAD_COLOR);
            klgl::ErrorHandling::Ensure(!image.empty(), "Failed to read image at {}", image_path);
            Vec2i image_size_i = Vec2i{image.cols, image.rows};
            Vec2f image_size_f = image_size_i.Cast<float>();
            Vec2i max_pixel_coord = image_size_i - 1;

            auto positions_path = GetExecutableDir() / AppGUI::kDefaultPositionsDumpFileName;
            std::ifstream positions_file(positions_path);
            klgl::ErrorHandling::Ensure(
                positions_file.is_open(),
                "Failed to open positions file at {}",
                positions_path);
            size_t count = 0;
            positions_file >> count;

            std::vector<edt::Vec3<uint8_t>> colors;
            colors.reserve(count);

            Vec2f min_coord = sim_area.Min();
            Vec2f coord_extent = sim_area.Extent();
            for (size_t i = 0; i != count; ++i)
            {
                edt::Vec2f position;
                positions_file >> position.x();
                positions_file >> position.y();

                // for give position find the closest image pixel
                auto rel = (position - min_coord) / coord_extent;
                auto pixel_coord = (rel * image_size_f).Cast<int>();
                pixel_coord = edt::Math::Clamp(pixel_coord, Vec2i{}, max_pixel_coord);
                pixel_coord.y() = max_pixel_coord.y() - pixel_coord.y();
                auto cv_color = image.at<cv::Vec3b>(pixel_coord.y(), pixel_coord.x());
                colors.push_back({cv_color[2], cv_color[1], cv_color[0]});
            }

            auto color_strategy = std::make_unique<SpawnColorStrategyArray>(*this);
            color_strategy->colors = std::move(colors);
            return color_strategy;
        }();

        solver.SetThreadsCount(1);
        for (auto& emitter : GetEmitters())
        {
            emitter.SetFlag(EmitterFlag::Enabled, true);
        }

        frame_ = cv::Mat(window_size_i.y(), window_size_i.x(), CV_8UC3);

        frame_pixels_.resize(window_size.x() * window_size.y());
        const auto output_video_path =
            (GetExecutableDir() / fmt::format("{}{}", image_path.stem(), kOutputVideoFormat));
        fmt::println("Output file: {}", output_video_path);
        video_writer_ = std::make_unique<cv::VideoWriter>(
            output_video_path,
            FourCC(kVideoEncoding),
            kOutputVideoFPS,
            cv::Size(window_size_i.x(), window_size_i.y()));
    }

    void Tick() override
    {
        Super::Tick();

        if (video_writer_)
        {
            const auto window_size = GetWindow().GetSize();

            klgl::OpenGl::ReadPixels(
                0,
                0,
                window_size.x(),
                window_size.y(),
                GL_BGR,
                GL_UNSIGNED_BYTE,
                frame_pixels_.data());

            // Copy the pixel data to a cv::Mat object (OpenCV)
            std::memcpy(frame_.data, frame_pixels_.data(), std::span{frame_pixels_}.size_bytes());

            // OpenGL's origin is at the bottom-left corner, so we need to flip the image vertically
            cv::flip(frame_, frame_, 0);

            video_writer_->write(frame_);

            if (++frames_written_ == 2000)
            {
                video_writer_.release();
                video_writer_ = nullptr;
            }
        }
    }
};
}  // namespace verlet

void Main()
{
    verlet::VerletVideoApp app;
    app.Run();
}

int main()
{
    klgl::ErrorHandling::InvokeAndCatchAll(Main);
    return 0;
}
