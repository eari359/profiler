#pragma once
// Simple profiler based on The Cherno's video:
// https://www.youtube.com/watch?v=xlAH4dbMVnU
#include<algorithm>
#include<chrono>
#include<fstream>
#include<thread>

#include<cassert>

#define PROFILING 1
#if PROFILING
#define PROFILE_SCOPE(name) profiler::InstrumentationTimer timer##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)
#else
#define PROFILE_SCOPE(name)
#endif

namespace profiler
{
	struct ProfileResult
	{
		char const* name;
		long long start;
		long long end;
		uint32_t thread_id;
	};

	class Instrumentor
	{
		public:
			static Instrumentor& Instance()
			{
				static Instrumentor instance;
				return instance;
			}

			void beginSession(char const* file_path = "profile.json")
			{
				output_stream_.open(file_path);
				writeHeader();
			}

			void endSession()
			{
				writeFooter();
				output_stream_.close();
				first_profile = true;
			}

			void writeProfile(ProfileResult const& result)
			{
				assert(output_stream_.is_open());

				if (!first_profile) {
					output_stream_ << ",";
				}

				first_profile = false;

				std::string name = result.name;
				std::replace(name.begin(), name.end(), '"', '\'');

				output_stream_ << '{';
				output_stream_ << R"("cat":"function",)";
				output_stream_ << R"("dur":)" << (result.end - result.start) << ',';
				output_stream_ << R"("name":")" << name << R"(",)";
				output_stream_ << R"("ph":"X",)";
				output_stream_ << R"("pid":0,)";
				output_stream_ << R"("tid":)" << result.thread_id << ',';
				output_stream_ << R"("ts":)" << result.start;
				output_stream_ << '}';


				output_stream_.flush();
			}

			void writeHeader()
			{
				assert(output_stream_.is_open());
				output_stream_ << "{\"otherData\": {},\"traceEvents\":[";
			}

			void writeFooter()
			{
				assert(output_stream_.is_open());
				output_stream_ << "]}";
				output_stream_.flush();
			}

		protected:
			Instrumentor() {}

		private:
			std::ofstream output_stream_;
			bool first_profile{ true };
	};

	class InstrumentationTimer
	{
		using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

		public:
		explicit InstrumentationTimer(char const* name)
			: name_{ name }
		{
			start_ = std::chrono::system_clock::now();
		}

		~InstrumentationTimer()
		{
			long long start =
				std::chrono::time_point_cast<std::chrono::microseconds>(start_)
				.time_since_epoch().count();
			long long end =
				std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now())
				.time_since_epoch().count();

			uint32_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

			Instrumentor::Instance().writeProfile({ name_, start, end, thread_id });
		}

		private:
		TimePoint start_;
		char const* name_;
	};
}
