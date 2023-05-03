#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>

#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml("../demo/config.tsv", "CN", 1111);
    dsml.set("IMAGE_SENT", (uint8_t)0);

    std::cout << "Press any key to start the demo...\n";
    std::string x;
    std::getline(std::cin, x);

    std::cout << "Starting video unit\n";

    dsml.register_owner("DN", "127.0.0.1", 1112);

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Couldn't open video capture device\n";
        return -1;
    }

    while (true) {
        cv::Mat frame, gray;
        cap >> frame;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        std::vector<uint8_t> image_data (gray.data, gray.data + gray.rows * gray.cols);
        dsml.set("IMAGE_DATA", image_data);
        dsml.set("IMAGE_ROWS", gray.rows);
        dsml.set("IMAGE_COLS", gray.cols);
        dsml.set("IMAGE_SENT", (uint8_t)1);

        auto det0 = dsml.get<std::vector<double>>("DET_POINT_1");
        auto det1 = dsml.get<std::vector<double>>("DET_POINT_2");
        auto det2 = dsml.get<std::vector<double>>("DET_POINT_3");
        auto det3 = dsml.get<std::vector<double>>("DET_POINT_4");

        if (det0[0] == -1) {
            cv::imshow("Video", frame);
            if (cv::waitKey(30) >= 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(41666));
            continue;
        }

        cv::line(frame, cv::Point(det0[0], det0[1]),
                     cv::Point(det1[0], det1[1]),
                     cv::Scalar(0, 0xff, 0), 8);
        cv::line(frame, cv::Point(det0[0], det0[1]),
                     cv::Point(det3[0], det3[1]),
                    cv::Scalar(0, 0, 0xff), 8);
        cv::line(frame, cv::Point(det1[0], det1[1]),
                     cv::Point(det2[0], det2[1]),
                    cv::Scalar(0xff, 0, 0), 8);
        cv::line(frame, cv::Point(det2[0], det2[1]),
                     cv::Point(det3[0], det3[1]),
                    cv::Scalar(0xff, 0, 0), 8);

        cv::imshow("Video", frame);
        if (cv::waitKey(30) >= 0) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(41666));
    }

    return 0;
}
