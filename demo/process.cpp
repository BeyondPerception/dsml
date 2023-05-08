#include <iostream>

#include "apriltag_pose.h"
#include "apriltag.h"
#include "tag36h11.h"

#include <dsml.hpp>

int main(int argc, char *argv[])
{
    dsml::State dsml("../demo/config.tsv", "DN", 1112);

    dsml.set("DET_POINT_1", std::vector<double>{-1, -1});
    dsml.set("DET_POINT_2", std::vector<double>{-1, -1});
    dsml.set("DET_POINT_3", std::vector<double>{-1, -1});
    dsml.set("DET_POINT_4", std::vector<double>{-1, -1});

    std::cout << "Press any key to start the demo...\n";
    std::string x;
    std::getline(std::cin, x);

    std::cout << "Starting processing unit\n";

    dsml.register_owner("CN", "127.0.0.1", 1111);
    auto sent = dsml.get<uint8_t>("IMAGE_SENT");

    apriltag_family_t *tf = tag36h11_create();
    apriltag_detector_t *td = apriltag_detector_create();
    apriltag_detector_add_family(td, tf);

    while (true)
    {
        dsml.wait("IMAGE_SENT");

        auto image_data = dsml.get<std::vector<uint8_t>>("IMAGE_DATA");
        auto rows = dsml.get<int>("IMAGE_ROWS");
        auto cols = dsml.get<int>("IMAGE_COLS");

        image_u8_t im = {
            .width = cols,
            .height = rows,
            .stride = cols,
            .buf = image_data.data(),
        };

        zarray_t *detections = apriltag_detector_detect(td, &im);
        if (zarray_size(detections) == 0)
        {
            dsml.set("DET_POINT_1", std::vector<double>{-1, -1});
            dsml.set("DET_POINT_2", std::vector<double>{-1, -1});
            dsml.set("DET_POINT_3", std::vector<double>{-1, -1});
            dsml.set("DET_POINT_4", std::vector<double>{-1, -1});
            continue;
        }

        apriltag_detection *det;
        zarray_get(detections, 0, &det);

        dsml.set("DET_POINT_1", std::vector<double>{det->p[0][0], det->p[0][1]});
        dsml.set("DET_POINT_2", std::vector<double>{det->p[1][0], det->p[1][1]});
        dsml.set("DET_POINT_3", std::vector<double>{det->p[2][0], det->p[2][1]});
        dsml.set("DET_POINT_4", std::vector<double>{det->p[3][0], det->p[3][1]});

        apriltag_detections_destroy(detections);
    }

    apriltag_detector_destroy(td);
    tag36h11_destroy(tf);

    return 0;
}
