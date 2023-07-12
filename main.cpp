#include "SafeQueue.h"

#include <cast/cast.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <igtlImageMessage.h>
#include <igtlServerSocket.h>
#include <iostream>
#include <mutex>


using namespace std::chrono_literals;

void IGTServer(SafeQueue<igtl::ImageMessage::Pointer> &queue, int port) {
    igtl::ServerSocket::Pointer serverSocket;
    while (true) {
        serverSocket = igtl::ServerSocket::New();
        int r = serverSocket->CreateServer(port);

        if (r < 0) {
            std::cerr << "Cannot create a server socket. Trying next port..." << std::endl;
            port++;
        } else {
            std::cout << "Created IGTLink server on port: " << port << std::endl;
            break;
        }
    }

    std::cout << "Server created correctly" << std::endl;

    igtl::Socket::Pointer socket;
    while (true) {
        socket = serverSocket->WaitForConnection(1000);

        if (socket.IsNotNull())    // if client connected
        {
            std::cout << "Got a connection" << std::endl;

            while (true) {
                auto imgMsg = queue.dequeue();

                if (imgMsg == nullptr) {
                    std::cerr << "IGTLink Closing connection" << std::endl;
                    break;
                }

                imgMsg->Pack();
                socket->Send(imgMsg->GetPackPointer(), imgMsg->GetPackSize());
            }

            socket->CloseSocket();
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Usage: ClariusOpenIGTLinkBridge <ip> <port>" << std::endl;
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    std::cout << "IP: " << ip << " port: " << port << std::endl;

    static std::atomic<bool> run{false};
    static SafeQueue<igtl::ImageMessage::Pointer> queue;

    static long lastTimestamp = 0;

    bool ok = cusCastInit(
            0,
            nullptr,
            "/tmp/",
            // New processed image function callback
            [](const void *newImage, const CusProcessedImageInfo *nfo, int npos, const CusPosInfo *pos) {
                if (!nfo)
                    return;

                long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                std::cout << "Time difference: " << (timestamp - lastTimestamp) << "ms" << std::endl;
                lastTimestamp = timestamp;

                int size[] = {nfo->width, nfo->height, 1};          // image dimension
                float spacing[] = {1.0, 1.0, 5.0};                  // spacing (mm/pixel)
                int svsize[] = {nfo->width, nfo->height, 1};        // sub-volume size
                int svoffset[] = {0, 0, 0};                         // sub-volume offset
                int scalarType = igtl::ImageMessage::TYPE_UINT8;    // scalar type

                //------------------------------------------------------------
                // Create a new IMAGE type message
                igtl::ImageMessage::Pointer imgMsg = igtl::ImageMessage::New();
                imgMsg->SetDimensions(size);
                imgMsg->SetSpacing(spacing);
                imgMsg->SetScalarType(scalarType);
                imgMsg->SetDeviceName("Clarius");
                imgMsg->SetSubVolume(svsize, svoffset);
                imgMsg->SetNumComponents(nfo->bitsPerPixel / 8);
                imgMsg->AllocateScalars();

                std::memcpy(imgMsg->GetScalarPointer(), newImage, nfo->imageSize);

                igtl::Matrix4x4 matrix;
                igtl::IdentityMatrix(matrix);
                imgMsg->SetMatrix(matrix);

                queue.enqueue(imgMsg);
            },
            // New raw image function callback
            [](const void * /*newImage*/, const CusRawImageInfo *nfo, int /*npos*/, const CusPosInfo * /*pos*/) {
            },
            // New spectral image function callback
            [](const void * /*newImage*/, const CusSpectralImageInfo * /*nfo*/) {},
            // Freeze / unfreeze function callback
            [](int val) {
                if (val)
                    std::cout << "Frozen" << std::endl;
                else
                    std::cout << "Imaging" << std::endl;
            },
            // Button function callback
            [](CusButton btn, int clicks) {
                std::cout << "Button event (button: " << static_cast<int>(btn) << ", clicks: " << clicks << ")" << std::endl;
            },
            // Progress function callback
            [](int /*progress*/) {},
            // Error function callback
            [](const char *err) { std::cerr << "Clarius Cast reported error: " << err << std::endl; },
            640,
            480) >= 0;
    if (!ok)
        std::cout << "ERROR in cusCastInit";

    ok = cusCastConnect(ip, port, "research", [](int port, int swRevMatch) {
        if (swRevMatch == CUS_SUCCESS)
            std::cout << "Clarius Cast connection OK" << std::endl;
        else
            std::cerr << "Clarius Cast version mismatch, ClariusOpenIGTLinkBridge was build with " << CLARIUS_CAST_VERSION
                      << ". Please follow the instructions at https://github.com/ImFusionGmbH/ClariusOpenIGTLinkBridge#updating-cast-api-version to update the Cast version.";
    }) >= 0;
    if (!ok)
        std::cout << "ERROR in cusCastConnect";

    IGTServer(queue, 8080);

    return 0;
}
