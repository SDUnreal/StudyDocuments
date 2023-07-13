/// SDUnreal Test program

#include "main.h"

#include <iostream>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>

// this is mediapipe part
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"

#include "mediapipe/framework/port/opencv_highgui_inc.h"
#include "mediapipe/framework/port/opencv_imgproc_inc.h"

mediapipe::Status run() {
    using namespace std;
    using namespace mediapipe;

    // this is calculator part
    string protoG = R"(
            input_stream: "in",
            output_stream: "out",
            node {
                calculator: "PassThroughCalculator",
                input_stream: "in",
                output_stream: "out",
            }
            )";

    //graph init
    CalculatorGraphConfig config;
    if(!ParseTextProto<mediapipe::CalculatorGraphConfig>(protoG, &config))
        return absl::InternalError("SDUnreal : Calculator Parse failure");

    CalculatorGraph graph;
    MP_RETURN_IF_ERROR(graph.Initialize(config));


    //call back
    auto cb = [](const Packet &packet)->Status{
        cout << packet.Timestamp() << ": RECEIVED VIDEO PACKET !" << endl;

        //open cv part : show image
        const ImageFrame & outputFrame = packet.Get<ImageFrame>();
        cv::Mat ofMat = formats::MatView(&outputFrame);
        cv::Mat frameOut;
        cvtColor(ofMat, frameOut, cv::COLOR_RGB2BGR);
        cv::imshow("frameOut", frameOut);
        if (27 == cv::waitKey(1))
            return absl::CancelledError("It's time to QUIT !");
        else
            return OkStatus();
    };
    // assign callback whenever graph give us packet 
    MP_RETURN_IF_ERROR(graph.ObserveOutputStream("out", cb));
    // run graph
    graph.StartRun({});

    cv::VideoCapture cap(cv::CAP_ANY);
    if(!cap.isOpened())
        return absl::NotFoundError("SDUnreal : Camera Init failure");
    cv::Mat frameIn, frameInRGB;

    uint64 ts = 1;
    
    while(true)
    {
        cap.read(frameIn);
        if(frameIn.empty())
            return absl::NotFoundError("SDUnreal : Camera Capture Failure");
        
        //Color convert (I don't know why we do this..)
        cv::cvtColor(frameIn, frameInRGB, cv::COLOR_BGR2RGB);

        // I think it is input frame init..
        ImageFrame *inputFrame = new ImageFrame(
            ImageFormat::SRGB, frameInRGB.cols, frameInRGB.rows, ImageFrame::kDefaultAlignmentBoundary
        );

        frameInRGB.copyTo(formats::MatView(inputFrame));

        

        //Adopt create a new packet from raw pointer It looks like shared_ptr you must not delete on the pointer
        MP_RETURN_IF_ERROR(graph.AddPacketToInputStream("in",
            Adopt(inputFrame).At(Timestamp(ts))));
        ts++;

    }
}
int startFunction(int argc, char** argv)
{
    using namespace std;
    cout <<"SD Unreal Open cv test" << endl;
    mediapipe::Status status = run();
    cout << "status =" << status << endl;
    cout << "status.ok()" << status.ok() << endl;

    return 0;
}
