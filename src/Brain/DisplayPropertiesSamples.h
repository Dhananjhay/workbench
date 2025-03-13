#ifndef __DISPLAY_PROPERTIES_SAMPLES_H__
#define __DISPLAY_PROPERTIES_SAMPLES_H__

/*LICENSE_START*/
/*
 *  Copyright (C) 2015 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/


#include "BrainConstants.h"
#include "DisplayGroupEnum.h"
#include "DisplayProperties.h"
#include "EventListenerInterface.h"
#include "SamplesColorModeEnum.h"


namespace caret {
    class Annotation;
    class Brain;

    class DisplayPropertiesSamples : public DisplayProperties, public EventListenerInterface {
        
    public:
        DisplayPropertiesSamples(Brain* parentBrain);
        
        virtual ~DisplayPropertiesSamples();
        
        virtual void receiveEvent(Event* event);
        
        virtual void reset();
        
        virtual void update();
        
        bool isDisplaySamples() const;
        
        void setDisplaySamples(const bool status);
        
        bool isDisplaySampleNames() const;
        
        void setDisplaySampleNames(const bool status);
        
        bool isDisplaySampleNumbers() const;
        
        void setDisplaySampleNumbers(const bool status);
        
        bool isDisplaySampleActualDesiredSuffix() const;
        
        void setDisplaySampleActualDesiredSuffix(const bool status);
        
        SamplesColorModeEnum::Enum getColorMode() const;
        
        void setColorMode(const SamplesColorModeEnum::Enum colorMode);
        
        DisplayGroupEnum::Enum getDisplayGroupForTab(const int32_t browserTabIndex) const;
        
        void setDisplayGroupForTab(const int32_t browserTabIndex,
                                   const DisplayGroupEnum::Enum displayGroup);
                
        void updateForNewSamples(const std::vector<Annotation*>& samples);
        
        void updateForNewSample(const Annotation* sample);
        
        virtual void copyDisplayProperties(const int32_t sourceTabIndex,
                                           const int32_t targetTabIndex);

        virtual SceneClass* saveToScene(const SceneAttributes* sceneAttributes,
                                        const AString& instanceName);
        
        virtual void restoreFromScene(const SceneAttributes* sceneAttributes,
                                      const SceneClass* sceneClass);
        
        // ADD_NEW_METHODS_HERE

    private:
        DisplayPropertiesSamples(const DisplayPropertiesSamples&);

        DisplayPropertiesSamples& operator=(const DisplayPropertiesSamples&);
        
        void resetPrivate();
        
        Brain* m_parentBrain;
        
        bool m_displaySamples;
        
        bool m_displaySampleNames;
        
        bool m_displaySampleNumbers;
        
        bool m_displaySampleActualDesiredSuffix;
        
        SamplesColorModeEnum::Enum m_colorMode = SamplesColorModeEnum::SAMPLE;
        
        DisplayGroupEnum::Enum m_displayGroup[BrainConstants::MAXIMUM_NUMBER_OF_BROWSER_TABS];
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __DISPLAY_PROPERTIES_SAMPLES_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __DISPLAY_PROPERTIES_SAMPLES_DECLARE__

} // namespace
#endif  //__DISPLAY_PROPERTIES_SAMPLES_H__
