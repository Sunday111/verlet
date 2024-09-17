#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "verlet/verlet_app.hpp"

int main([[maybe_unused]] int argc, const char** argv)
{
    // Video properties
    int frameWidth = 640;
    int frameHeight = 480;
    int fps = 30;
    int totalFrames = 100;  // Generate 100 frames

    std::string file_path = argv[0];  // NOLINT
    file_path.append("/output.mp4");

    cv::VideoWriter videoWriter(
        file_path,
        cv::VideoWriter::fourcc('H', '2', '6', '4'),
        fps,
        cv::Size(frameWidth, frameHeight));

    // Check if the VideoWriter object was initialized correctly
    if (!videoWriter.isOpened())
    {
        return -1;
    }

    cv::namedWindow("Video", cv::WINDOW_AUTOSIZE);

    // Generate frames and write to the video
    for (int i = 0; i < totalFrames; i++)
    {
        // Create a blank image (black background)
        cv::Mat frame = cv::Mat::zeros(frameHeight, frameWidth, CV_8UC3);

        // Define a moving circle's position and color
        int x = (frameWidth / totalFrames) * i;    // Horizontal movement
        int y = frameHeight / 2;                   // Fixed vertical position
        cv::Scalar color = cv::Scalar(0, 255, 0);  // Green color (BGR format)

        // Draw the circle on the frame
        cv::circle(frame, cv::Point(x, y), 50, color, -1);  // Circle radius = 50, filled (-1 thickness)

        // Write the frame to the video
        videoWriter.write(frame);

        // Display the frame (optional, just for visual feedback)
        cv::imshow("Video", frame);

        // Wait for a short moment (33ms) to simulate real-time video playback
        if (cv::waitKey(33) >= 0) break;  // Break on any key press
    }

    // Release the video writer
    videoWriter.release();

    // Close the window
    cv::destroyWindow("Video");

    return 0;
}
