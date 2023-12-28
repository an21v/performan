#include "gtest/gtest.h"

#include "performan.h"

#include <cassert>
#include <cstdint>
#include <iostream>

void Assert(const char* condition, const char* filename, const char* line, int linenumber) {
    std::cout << "[Assert]: " << condition << " (" << filename << ": " << line << "::" << linenumber << ")" << std::endl;
}

class PerformanTest : public testing::Test {
protected:
    void SetUp() override {
        Performan::PerformanSetAssertFunction(&Assert);
    }

    void TearDown() override {
        // Nothing yet
    }
};

TEST_F(PerformanTest, TestEventName) {
    constexpr const char* evtName = "evt";
    Performan::Event evt(evtName);
    EXPECT_EQ(evt._name, evtName);
}

TEST_F(PerformanTest, TestResizeFromEmpty) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::Stream stream(&allocator);

    stream.Resize();

    EXPECT_NE(stream.Data(), nullptr);
    EXPECT_EQ(stream.Size(), 1024);
}

TEST_F(PerformanTest, TestResizeTwice) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::Stream stream(&allocator);

    stream.Resize();

    EXPECT_NE(stream.Data(), nullptr);
    EXPECT_EQ(stream.Size(), 1024);

    stream.Resize();

    EXPECT_NE(stream.Data(), nullptr);
    EXPECT_EQ(stream.Size(), 2048);
}

TEST_F(PerformanTest, TestResizeDataCorruption) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::Stream stream(&allocator);

    stream.Resize();
    uint8_t* buf = stream.Data();

    buf[0] = 'a';
    buf[1] = 'l';
    buf[2] = 'l';
    buf[3] = 'o';

    EXPECT_NE(stream.Data(), nullptr);
    EXPECT_EQ(stream.Size(), 1024);

    stream.Resize();

    EXPECT_NE(stream.Data(), nullptr);
    EXPECT_EQ(stream.Size(), 2048);
    EXPECT_EQ(stream.Data()[0], 'a');
    EXPECT_EQ(stream.Data()[1], 'l');
    EXPECT_EQ(stream.Data()[2], 'l');
    EXPECT_EQ(stream.Data()[3], 'o');
}

TEST_F(PerformanTest, TestStreamSerializeInt64Min) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    int64_t valueToSerialize = INT64_MIN;
    PERFORMAN_SERIALIZE(wStream, &valueToSerialize, sizeof(int64_t));

    int64_t valueToDeserialize = 0;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    PERFORMAN_SERIALIZE(rStream, &valueToDeserialize, sizeof(int64_t));

    EXPECT_EQ(valueToSerialize, valueToDeserialize);

}

TEST_F(PerformanTest, TestStreamSerializeInt64Max) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    int64_t valueToSerialize = INT64_MAX;
    PERFORMAN_SERIALIZE(wStream, &valueToSerialize, sizeof(int64_t));

    int64_t valueToDeserialize = 0;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    PERFORMAN_SERIALIZE(rStream, &valueToDeserialize, sizeof(int64_t));

    EXPECT_EQ(valueToSerialize, valueToDeserialize);

}

TEST_F(PerformanTest, TestStreamSerializeInt64Value) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    int64_t valueToSerialize = 12345789;
    PERFORMAN_SERIALIZE(wStream, &valueToSerialize, sizeof(int64_t));

    int64_t valueToDeserialize = 0;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    PERFORMAN_SERIALIZE(rStream, &valueToDeserialize, sizeof(int64_t));

    EXPECT_EQ(valueToSerialize, valueToDeserialize);
}

TEST_F(PerformanTest, TestStreamSerializeEvent) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    Performan::Event evtSerialize("Test");
    evtSerialize._start = std::chrono::steady_clock::now();
    evtSerialize._end = std::chrono::steady_clock::now();
    evtSerialize.Serialize(wStream);

    Performan::Event evtDeserialize;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    evtDeserialize.Serialize(rStream);

    EXPECT_EQ(evtSerialize._start, evtDeserialize._start);
    EXPECT_EQ(evtSerialize._end, evtDeserialize._end);
    EXPECT_STREQ(evtSerialize._name, evtDeserialize._name);
}

TEST_F(PerformanTest, TestStreamSerializeEventEmpty) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    Performan::Event evtSerialize;
    evtSerialize.Serialize(wStream);

    Performan::Event evtDeserialize;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    evtDeserialize.Serialize(rStream);

    EXPECT_EQ(evtSerialize._start, evtDeserialize._start);
    EXPECT_EQ(evtSerialize._end, evtDeserialize._end);
    EXPECT_STREQ(evtSerialize._name, evtDeserialize._name);
}

TEST_F(PerformanTest, TestStreamSerializeFrameNoEvents) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    Performan::Frame frameSerialize;
    frameSerialize._start = std::chrono::steady_clock::now();
    frameSerialize._end = std::chrono::steady_clock::now();
    frameSerialize.Serialize(wStream);

    Performan::Frame frameDeserialize;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    frameDeserialize.Serialize(rStream);

    EXPECT_EQ(frameSerialize._start, frameDeserialize._start);
    EXPECT_EQ(frameSerialize._end, frameDeserialize._end);
}

TEST_F(PerformanTest, TestStreamSerializeFrame) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    Performan::Frame frameSerialize;
    frameSerialize._start = std::chrono::steady_clock::now();
    frameSerialize._end = std::chrono::steady_clock::now();

    frameSerialize.Serialize(wStream);

    Performan::Frame frameDeserialize;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    frameDeserialize.Serialize(rStream);

    EXPECT_EQ(frameSerialize._start, frameDeserialize._start);
    EXPECT_EQ(frameSerialize._end, frameDeserialize._end);
}

TEST_F(PerformanTest, TestStreamSerializeThreadNoFrame) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    Performan::Thread threadSerialize;
    threadSerialize.Serialize(wStream);

    Performan::Thread threadDeserialize;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    threadDeserialize.Serialize(rStream);

    EXPECT_STREQ(threadSerialize._name, threadDeserialize._name);
    EXPECT_EQ(threadSerialize._frames.size(), threadDeserialize._frames.size());
    EXPECT_EQ(threadSerialize._events.size(), threadDeserialize._events.size());
}

TEST_F(PerformanTest, TestStreamSerializeThread) {
    Performan::Allocator& allocator = Performan::GetDefaultAllocator();
    Performan::WriteStream wStream(&allocator);

    Performan::Thread threadSerialize;
    threadSerialize._name = "MainThread";

    Performan::Frame frameSerialize;
    threadSerialize._frames.push_back(frameSerialize);

    Performan::Event evtSerialize;
    evtSerialize._name = "Event";
    threadSerialize._events.push_back(evtSerialize);

    threadSerialize.Serialize(wStream);

    Performan::Thread threadDeserialize;
    Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
    threadDeserialize.Serialize(rStream);

    EXPECT_STREQ(threadSerialize._name, threadDeserialize._name);
    EXPECT_EQ(threadSerialize._frames.size(), threadDeserialize._frames.size());
    EXPECT_EQ(threadSerialize._events.size(), threadDeserialize._events.size());
}

