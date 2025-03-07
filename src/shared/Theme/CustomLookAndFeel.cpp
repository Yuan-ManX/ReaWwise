#include "CustomLookAndFeel.h"

#include "BinaryData.h"

namespace AK::WwiseTransfer
{
	CustomLookAndFeel::CustomLookAndFeel()
		: windowBackgroundColor{0xff464646}
		, widgetBackgroundColor{0xff373737}
		, thickOutlineColor{0xff3a3a3a}
		, textColor{0xfff9f9f9}
		, textColorDisabled(0x99000000)
		, highlightedTextColor{0xffbcbcbc}
		, highlightedFillColor{0xff646464}
		, buttonBackgroundColor{0xff5a5a5a}
		, thinOutlineColor{0xff292929}
		, tableHeaderBackgroundColor{0xff545454}
		, previewItemNoChangeColor{0xff7d7d7d}
		, previewItemNewColor{0xff29afff}
		, previewItemReplacedColor{0xffda8e40}
		, regularTypeFace{juce::Typeface::createSystemTypefaceFor(BinaryData::open_sans_ttf, BinaryData::open_sans_ttfSize)}
		, boldTypeFace{juce::Typeface::createSystemTypefaceFor(BinaryData::open_sans_bold_ttf, BinaryData::open_sans_bold_ttfSize)}
	{
		setColourScheme({
			windowBackgroundColor,
			widgetBackgroundColor,
			widgetBackgroundColor,
			thickOutlineColor,
			textColor,
			highlightedFillColor,
			highlightedTextColor,
			highlightedFillColor,
			textColor,
		});

		setColour(juce::TextButton::buttonColourId, buttonBackgroundColor);
		setColour(juce::TextEditor::outlineColourId, thinOutlineColor);
		setColour(juce::ComboBox::outlineColourId, thinOutlineColor);
		setColour(juce::TreeView::backgroundColourId, widgetBackgroundColor);
		setColour(juce::TableHeaderComponent::backgroundColourId, tableHeaderBackgroundColor);
		setColour(juce::TableHeaderComponent::textColourId, textColor);
		setColour(juce::TableHeaderComponent::highlightColourId, highlightedFillColor);
		setColour(juce::TableHeaderComponent::outlineColourId, thickOutlineColor);
		setColour(juce::HyperlinkButton::textColourId, textColor);
		setColour(juce::TooltipWindow::backgroundColourId, widgetBackgroundColor);
	}

	const std::shared_ptr<juce::Drawable>& CustomLookAndFeel::getIconForObjectType(Wwise::ObjectType objectType)
	{
		using namespace Wwise;

		switch(objectType)
		{
		case ObjectType::ActorMixer:
		{
			static std::shared_ptr<juce::Drawable> actorMixerIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_PhysicalFolder_nor_svg, BinaryData::ObjectIcons_PhysicalFolder_nor_svgSize));
			return actorMixerIcon;
		}
		case ObjectType::AudioFileSource:
		{
			static std::shared_ptr<juce::Drawable> actorMixerIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_AudioObjectSound_nor_svg, BinaryData::ObjectIcons_AudioObjectSound_nor_svgSize));
			return actorMixerIcon;
		}
		case ObjectType::BlendContainer:
		{
			static std::shared_ptr<juce::Drawable> blendContainerIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_BlendContainer_nor_svg, BinaryData::ObjectIcons_BlendContainer_nor_svgSize));
			return blendContainerIcon;
		}
		case ObjectType::PhysicalFolder:
		{
			static std::shared_ptr<juce::Drawable> physicalFolderIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_PhysicalFolder_nor_svg, BinaryData::ObjectIcons_PhysicalFolder_nor_svgSize));
			return physicalFolderIcon;
		}
		case ObjectType::RandomContainer:
		{
			static std::shared_ptr<juce::Drawable> randomContainerIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_RandomContainer_nor_svg, BinaryData::ObjectIcons_RandomContainer_nor_svgSize));
			return randomContainerIcon;
		}
		case ObjectType::SequenceContainer:
		{
			static std::shared_ptr<juce::Drawable> sequenceContainerIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_SequenceContainer_nor_svg, BinaryData::ObjectIcons_SequenceContainer_nor_svgSize));
			return sequenceContainerIcon;
		}
		case ObjectType::SoundSFX:
		{
			static std::shared_ptr<juce::Drawable> soundSFXIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_SoundFX_nor_svg, BinaryData::ObjectIcons_SoundFX_nor_svgSize));
			return soundSFXIcon;
		}
		case ObjectType::SoundVoice:
		{
			static std::shared_ptr<juce::Drawable> soundVoiceIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_SoundVoice_nor_svg, BinaryData::ObjectIcons_SoundVoice_nor_svgSize));
			return soundVoiceIcon;
		}
		case ObjectType::SwitchContainer:
		{
			static std::shared_ptr<juce::Drawable> switchContainerIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_SwitchContainer_nor_svg, BinaryData::ObjectIcons_SwitchContainer_nor_svgSize));
			return switchContainerIcon;
		}
		case ObjectType::VirtualFolder:
		{
			static std::shared_ptr<juce::Drawable> virtualFolderIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_Folder_nor_svg, BinaryData::ObjectIcons_Folder_nor_svgSize));
			return virtualFolderIcon;
		}
		case ObjectType::WorkUnit:
		{
			static std::shared_ptr<juce::Drawable> workUnitIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_Workunit_nor_svg, BinaryData::ObjectIcons_Workunit_nor_svgSize));
			return workUnitIcon;
		}
		default:
		{
			static std::shared_ptr<juce::Drawable> defaultIcon(juce::Drawable::createFromImageData(BinaryData::ObjectIcons_Folder_nor_svg, BinaryData::ObjectIcons_Folder_nor_svgSize));
			return defaultIcon;
		}
		}
	}

	juce::Colour CustomLookAndFeel::getTextColourForObjectStatus(Import::ObjectStatus objectStatus)
	{
		if(objectStatus == Import::ObjectStatus::NoChange)
			return previewItemNoChangeColor;
		else if(objectStatus == Import::ObjectStatus::New || objectStatus == Import::ObjectStatus::NewRenamed)
			return previewItemNewColor;
		else if(objectStatus == Import::ObjectStatus::Replaced)
			return previewItemReplacedColor;
		else
			return textColor;
	}

	void CustomLookAndFeel::drawTableHeaderColumn(juce::Graphics& g, juce::TableHeaderComponent& header,
		const juce::String& columnName, int /*columnId*/,
		int width, int height, bool isMouseOver, bool isMouseDown,
		int columnFlags)
	{
		auto highlightColour = header.findColour(juce::TableHeaderComponent::highlightColourId);
		auto columnDisabled = (columnFlags & 128) != 0;

		if(isMouseDown && !columnDisabled)
			g.fillAll(highlightColour);
		else if(isMouseOver && !columnDisabled)
			g.fillAll(highlightColour.withMultipliedAlpha(0.625f));

		juce::Rectangle<int> area(width, height);
		area.reduce(4, 0);

		if((columnFlags & (juce::TableHeaderComponent::sortedForwards | juce::TableHeaderComponent::sortedBackwards)) != 0)
		{
			juce::Path sortArrow;
			sortArrow.addTriangle(0.0f, 0.0f,
				0.5f, (columnFlags & juce::TableHeaderComponent::sortedForwards) != 0 ? -0.8f : 0.8f,
				1.0f, 0.0f);

			g.setColour(textColorDisabled);
			g.fillPath(sortArrow, sortArrow.getTransformToScaleToFit(area.removeFromRight(height / 2).reduced(2).toFloat(), true));
		}

		auto textColour = header.findColour(juce::TableHeaderComponent::textColourId)
		                      .withAlpha(columnDisabled ? 0.5f : 1.0f);
		g.setColour(textColour);
		g.setFont(getTableHeaderFont().withHeight((float)height * 0.6f));
		g.drawFittedText(columnName, area, juce::Justification::centredLeft, 1);
	}

	void CustomLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int width, int height,
		const juce::String& text, const juce::Justification& position,
		juce::GroupComponent& group)
	{
		const float indent = 3.0f;
		const float textEdgeGap = 4.0f;
		auto constant = 5.0f;

		juce::Font f(CustomLookAndFeelConstants::regularFontSize);

		juce::Path p;
		auto x = indent;
		auto y = f.getAscent() - 3.0f;
		auto w = juce::jmax(0.0f, (float)width - x * 2.0f);
		auto h = juce::jmax(0.0f, (float)height - y - indent);
		constant = juce::jmin(constant, w * 0.5f, h * 0.5f);
		auto constant2 = 2.0f * constant;

		auto textW = text.isEmpty() ? 0 : juce::jlimit(0.0f, juce::jmax(0.0f, w - constant2 - textEdgeGap * 2), (float)f.getStringWidth(text) + textEdgeGap * 2.0f);
		auto textX = constant + textEdgeGap;

		if(position.testFlags(juce::Justification::horizontallyCentred))
			textX = constant + (w - constant2 - textW) * 0.5f;
		else if(position.testFlags(juce::Justification::right))
			textX = w - constant - textW - textEdgeGap;

		p.startNewSubPath(x + textX + textW, y);
		p.lineTo(x + w - constant, y);

		p.addArc(x + w - constant2, y, constant2, constant2, 0, juce::MathConstants<float>::halfPi);
		p.lineTo(x + w, y + h - constant);

		p.addArc(x + w - constant2, y + h - constant2, constant2, constant2, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi);
		p.lineTo(x + constant, y + h);

		p.addArc(x, y + h - constant2, constant2, constant2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 1.5f);
		p.lineTo(x, y + constant);

		p.addArc(x, y, constant2, constant2, juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::twoPi);
		p.lineTo(x + textX, y);

		auto alpha = group.isEnabled() ? 1.0f : 0.5f;

		g.setColour(group.findColour(juce::GroupComponent::outlineColourId)
						.withMultipliedAlpha(alpha));

		g.strokePath(p, juce::PathStrokeType(2.0f));

		g.setColour(group.findColour(juce::GroupComponent::textColourId)
						.withMultipliedAlpha(alpha));
		g.setFont(f);
		g.drawText(text,
			juce::roundToInt(x + textX), 0,
			juce::roundToInt(textW),
			juce::roundToInt(f.getHeight()),
			juce::Justification::centred, true);
	}

	juce::Typeface::Ptr CustomLookAndFeel::getTypefaceForFont(const juce::Font& font)
	{
		if(font.isBold())
			return boldTypeFace;

		return regularTypeFace;
	}

	void CustomLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
	{
		if(dynamic_cast<juce::AlertWindow*>(textEditor.getParentComponent()) != nullptr || !textEditor.isEnabled() || textEditor.isReadOnly())
			return;

		const auto hasKeyboardFocus = textEditor.hasKeyboardFocus(true);

		auto colour = textEditor.findColour(hasKeyboardFocus ? juce::TextEditor::focusedOutlineColourId : juce::TextEditor::outlineColourId);
		auto lineThickness = hasKeyboardFocus ? 2 : 1;

		g.setColour(colour);
		g.drawRect(0, 0, width, height, lineThickness);
	}

	void CustomLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
	{
		juce::Rectangle<int> bounds(width, height);

		g.setColour(findColour(juce::TooltipWindow::backgroundColourId));
		g.fillRect(bounds.toFloat());

		g.setColour(findColour(juce::TooltipWindow::outlineColourId));
		g.drawRect(bounds.toFloat().reduced(0.5f, 0.5f), 1.0f);

		const float tooltipFontSize = 13.0f;
		const int maxToolTipWidth = 400;

		juce::AttributedString s;
		s.setJustification(juce::Justification::centred);
		s.append(text, juce::Font(tooltipFontSize, juce::Font::bold), findColour(juce::TooltipWindow::textColourId));

		juce::TextLayout tl;
		tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);

		tl.draw(g, {static_cast<float>(width), static_cast<float>(height)});
	}

	juce::Font CustomLookAndFeel::getLabelFont(juce::Label& label)
	{
		static auto defaultLabelFont = juce::Label().getFont();

		if(label.getFont() != defaultLabelFont)
			return label.getFont();

		return juce::Font(CustomLookAndFeelConstants::regularFontSize);
	}

	juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
	{
		return juce::Font(CustomLookAndFeelConstants::smallFontSize);
	}

	juce::Font CustomLookAndFeel::getPopupMenuFont()
	{
		return juce::Font(CustomLookAndFeelConstants::regularFontSize);
	}

	juce::Font CustomLookAndFeel::getTableHeaderFont()
	{
		return juce::Font(CustomLookAndFeelConstants::smallFontSize, juce::Font::bold);
	}

	juce::Font CustomLookAndFeel::getAlertWindowFont()
	{
		return juce::Font(CustomLookAndFeelConstants::smallFontSize);
	}

	juce::Font CustomLookAndFeel::getAlertWindowMessageFont()
	{
		return juce::Font(CustomLookAndFeelConstants::smallFontSize);
	}

	juce::Font CustomLookAndFeel::getAlertWindowTitleFont()
	{
		return juce::Font(CustomLookAndFeelConstants::largeFontSize, juce::Font::bold);
	}
} // namespace AK::WwiseTransfer
