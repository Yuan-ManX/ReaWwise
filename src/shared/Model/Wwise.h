#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace AK::WwiseTransfer::Wwise
{
	enum class ImportObjectType
	{
		SFX,
		Voice,
		Music
	};

	enum class ImportOperation
	{
		CreateNew,
		UseExisting,
		ReplaceExisting
	};

	enum class ObjectType
	{
		Unknown,
		ActorMixer,
		AudioFileSource,
		BlendContainer,
		PhysicalFolder,
		RandomContainer,
		SequenceContainer,
		SoundSFX,
		SoundVoice,
		SwitchContainer,
		VirtualFolder,
		WorkUnit,
		Sound
	};

	struct Version
	{
		int year{};
		int major{};
		int minor{};
		int build{};

		bool operator<(const Version& other) const
		{
			return year < other.year ||
			       (year == other.year && major < other.major) ||
			       (year == other.year && major == other.major && minor < other.minor) ||
			       (year == other.year && major == other.major && minor == other.minor && build < other.build);
		}

		bool operator>(const Version& other) const
		{
			return other < *this;
		}

		bool operator<=(const Version& other) const
		{
			return !(*this > other);
		}

		bool operator>=(const Version& other) const
		{
			return !(*this < other);
		}

		bool operator==(const Version& other) const
		{
			return year == other.year && major == other.major && minor == other.minor && build == other.build;
		}

		bool operator!=(const Version& other) const
		{
			return !(*this == other);
		}

		friend juce::String& operator<<(juce::String& out, const Version& v)
		{
			out << v.year << "." << v.major << "." << v.minor << "." << v.build;
			return out;
		}
	};
} // namespace AK::WwiseTransfer::Wwise

template <>
struct juce::VariantConverter<AK::WwiseTransfer::Wwise::ObjectType>
{
	static AK::WwiseTransfer::Wwise::ObjectType fromVar(const juce::var v)
	{
		return static_cast<AK::WwiseTransfer::Wwise::ObjectType>(int(v));
	}

	static juce::var toVar(AK::WwiseTransfer::Wwise::ObjectType objectType)
	{
		return int(objectType);
	}
};
