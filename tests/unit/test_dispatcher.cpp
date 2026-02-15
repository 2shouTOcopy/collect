#include <gtest/gtest.h>
#include "core/Dispatcher.h"

/// Stub plugin for dispatcher tests.
class StubReadPlugin : public IPlugin
{
public:
	explicit StubReadPlugin(const std::string &name) : m_name(name) {}
	std::string Name() const override { return m_name; }
	bool HasRead() const override { return true; }
	int Read() override { return m_returnValue; }

	void SetReturnValue(int val) { m_returnValue = val; }

private:
	std::string m_name;
	int m_returnValue = 0;
};

TEST(DispatcherTest, RegisterAndTaskCount)
{
	Dispatcher disp;
	StubReadPlugin p1("cpu");
	StubReadPlugin p2("memory");

	disp.RegisterRead(&p1, CdTime::FromDouble(10.0));
	disp.RegisterRead(&p2, CdTime::FromDouble(30.0));

	EXPECT_EQ(disp.TaskCount(), 2u);
}

TEST(DispatcherTest, NullPluginIgnored)
{
	Dispatcher disp;
	disp.RegisterRead(nullptr, CdTime::FromDouble(10.0));
	EXPECT_EQ(disp.TaskCount(), 0u);
}

TEST(DispatcherTest, RunOnceEmptyReturnsDefaultWait)
{
	Dispatcher disp;
	CdTime wait = disp.RunOnce();
	EXPECT_DOUBLE_EQ(wait.ToDouble(), 1.0);
}
