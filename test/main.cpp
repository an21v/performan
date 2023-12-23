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


TEST_F(PerformanTest, TestSampleOne) {
	EXPECT_EQ(true, true);
}

TEST_F(PerformanTest, TestEventName) {
	constexpr const char* evtName = "evt";
	Performan::PM_Event evt(evtName);
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

TEST_F(PerformanTest, TestWriteStreamSerializeInt64Min) {
	Performan::Allocator& allocator = Performan::GetDefaultAllocator();
	Performan::WriteStream wStream(&allocator);

	int64_t valueToSerialize = INT64_MIN;
	wStream.SerializeInt64(valueToSerialize);

	int64_t valueToDeserialize = 0;
	Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
	rStream.SerializeInt64(valueToDeserialize);

	EXPECT_EQ(valueToSerialize, valueToDeserialize);

}

TEST_F(PerformanTest, TestWriteStreamSerializeInt64Max) {
	Performan::Allocator& allocator = Performan::GetDefaultAllocator();
	Performan::WriteStream wStream(&allocator);

	int64_t valueToSerialize = INT64_MAX;
	wStream.SerializeInt64(valueToSerialize);

	int64_t valueToDeserialize = 0;
	Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
	rStream.SerializeInt64(valueToDeserialize);

	EXPECT_EQ(valueToSerialize, valueToDeserialize);

}

TEST_F(PerformanTest, TestWriteStreamSerializeInt64Value) {
	Performan::Allocator& allocator = Performan::GetDefaultAllocator();
	Performan::WriteStream wStream(&allocator);

	int64_t valueToSerialize = 12345789;
	wStream.SerializeInt64(valueToSerialize);

	int64_t valueToDeserialize = 0;
	Performan::ReadStream rStream(&allocator, wStream.Data(), wStream.Size());
	rStream.SerializeInt64(valueToDeserialize);

	EXPECT_EQ(valueToSerialize, valueToDeserialize);
}
