
/*LICENSE_START*/
/*
 *  Copyright (C) 2024 Washington University School of Medicine
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

#define __LABEL_SELECTION_ITEM_MODEL_DECLARE__
#include "LabelSelectionItemModel.h"
#undef __LABEL_SELECTION_ITEM_MODEL_DECLARE__

#include <set>

#include "CaretAssert.h"
#include "CaretHierarchy.h"
#include "CaretLogger.h"
#include "ClusterContainer.h"
#include "GiftiLabel.h"
#include "GiftiLabelTable.h"
#include "GroupAndNameHierarchyItem.h"
#include "SceneClass.h"
#include "SceneClassAssistant.h"
#include "ScenePrimitiveArray.h"

using namespace caret;


    
/**
 * \class caret::LabelSelectionItemModel 
 * \brief Model for label hierarchy
 * \ingroup Files
 */

/**
 * Constructor.
 * @param fileAndMapName
 *    Name of file and map containing label table
 * @param GiftiLabelTable
 *    GIFTI label tabel from which this hierarchy is build
 * @param clusterContainer
 *    The cluster  container (may be NULL)
 * @param displayGroup
 *    The display group
 * @param tabIndex
 *    Index of the tab if displayGroup is TAB
 * @param logMismatchedLabelsFlag
 *    If true, log a message if any labels are in hierarchy but not in label table
 */
LabelSelectionItemModel::LabelSelectionItemModel(const AString& fileAndMapName,
                                                 GiftiLabelTable* giftiLabelTable,
                                                 const ClusterContainer* clusterContainer,
                                                 const DisplayGroupEnum::Enum displayGroup,
                                                 const int32_t tabIndex,
                                                 const bool logMismatchedLabelsFlag)
: QStandardItemModel(),
m_fileAndMapName(fileAndMapName),
m_giftiLabelTable(giftiLabelTable),
m_displayGroup(displayGroup),
m_tabIndex(tabIndex),
m_logMismatchedLabelsFlag(logMismatchedLabelsFlag)
{
    CaretAssert(m_giftiLabelTable);
    
    m_sceneAssistant = std::unique_ptr<SceneClassAssistant>(new SceneClassAssistant());
    
    buildModel(clusterContainer);
}

/**
 * Destructor.
 */
LabelSelectionItemModel::~LabelSelectionItemModel()
{
}

/**
 * @return True if this model is valid
 */
bool
LabelSelectionItemModel::isValid() const
{
    return m_validFlag;
}

/**
 * Set the checked status of all items
 * @param checked
 *    The checked status
 */
void
LabelSelectionItemModel::setCheckedStatusOfAllItems(const bool checked)
{
    QStandardItem* rootItem(invisibleRootItem());
    const int32_t numChildren(rootItem->rowCount());
    for (int32_t iRow = 0; iRow < numChildren; iRow++) {
        QStandardItem* childItem(rootItem->child(iRow));
        LabelSelectionItem* labelItem(dynamic_cast<LabelSelectionItem*>(childItem));
        CaretAssert(labelItem);
        labelItem->setCheckState(checked
                                 ? Qt::Checked
                                 : Qt::Unchecked);
        labelItem->setAllChildrenChecked(checked);
    }
}

/**
 * Update the checked state of all items
 */
void
LabelSelectionItemModel::updateCheckedStateOfAllItems()
{
    QStandardItem* rootItem(invisibleRootItem());
    const int32_t numChildren(rootItem->rowCount());
    for (int32_t iRow = 0; iRow < numChildren; iRow++) {
        QStandardItem* childItem(rootItem->child(iRow));
        LabelSelectionItem* labelItem(dynamic_cast<LabelSelectionItem*>(childItem));
        CaretAssert(labelItem);
        labelItem->setCheckStateFromChildren();
    }
}

/**
 * @return True if the label with the given key is checked, else false.
 * @param labelKey
 *    The label's key
 */
bool
LabelSelectionItemModel::isLabelChecked(const int32_t labelKey) const
{
    auto iter(m_labelKeyToLabelSelectionItem.find(labelKey));
    if (iter != m_labelKeyToLabelSelectionItem.end()) {
        const LabelSelectionItem* item(iter->second);
        if (item != NULL) {
            return (item->checkState() == Qt::Checked);
        }
    }
    return false;
}

/**
 * Build the tree model from the hierarchy and the label table
 * @param clusterContainer
 *    The cluster container (may be NULL)
 */
void
LabelSelectionItemModel::buildModel(const ClusterContainer* clusterContainer)
{
    CaretAssert(m_giftiLabelTable);
    
    m_labelKeyToLabelSelectionItem.clear();
    m_buildTreeMissingLabelNames.clear();
    
    const CaretHierarchy& caretHierarchy(m_giftiLabelTable->getHierarchy());
    if (caretHierarchy.isEmpty()) {
        return;
    }
    const CaretHierarchy::Item& caretRootItem(caretHierarchy.getInvisibleRoot());
    
    std::vector<LabelSelectionItem*> topLevelItems;
    const int32_t numChildren(caretRootItem.children.size());
    if (numChildren > 0) {
        for (int32_t i = 0; i < numChildren; i++) {
            CaretAssertVectorIndex(caretRootItem.children, i);
            topLevelItems.push_back(buildTree(&caretRootItem.children[i],
                                              m_giftiLabelTable,
                                              clusterContainer));
        }
    }
    
    AString text;
    
    /*
     * Name in hiearchy has not children and name is not found in label table
     */
    if ( ! m_buildTreeMissingLabelNames.empty()) {
        text.appendWithNewLine("   No labels in the label table were found for these childless elements in the hierarchy:");
        for (const AString& name : m_buildTreeMissingLabelNames) {
            text.appendWithNewLine("      " + name);
        }
    }
    
    /*
     * Name is in the label table but not found in the hierarchy
     * OR label is in hierarchy but has children
     * (Except for unassigned label key)
     */
    const int32_t unassignedLabelKey(m_giftiLabelTable->getUnassignedLabelKey());
    std::set<AString> buildTreeMissingHierarchyNames;
    std::set<AString> labelIsParentInHierarchyNames;
    const std::vector<int32_t> labelKeys(m_giftiLabelTable->getLabelKeysSortedByName());
    for (const int32_t key : labelKeys) {
        if (key != unassignedLabelKey) {
            if (m_labelKeyToLabelSelectionItem.find(key) == m_labelKeyToLabelSelectionItem.end()) {
                const AString labelName(m_giftiLabelTable->getLabelName(key));
                if (m_hierarchyParentNames.find(labelName) != m_hierarchyParentNames.end()) {
                    labelIsParentInHierarchyNames.insert(labelName);
                }
                else {
                    buildTreeMissingHierarchyNames.insert(labelName);
                }
            }
        }
    }
    
    if ( ! buildTreeMissingHierarchyNames.empty()) {
        /*
         * Add labels from label table that are not in the hierarchy to the hierarchy
         */
        const AString emptyOntologyID("");
        LabelSelectionItem* parentItem(new LabelSelectionItem("Label Table Only",
                                                              emptyOntologyID));
        for (const AString& name : buildTreeMissingHierarchyNames) {
            const GiftiLabel* giftiLabel(m_giftiLabelTable->getLabel(name));
            if (giftiLabel != NULL) {
                const int32_t labelKey(giftiLabel->getKey());
                LabelSelectionItem* item(new LabelSelectionItem(name,
                                                                emptyOntologyID,
                                                                labelKey,
                                                                getLabelRGBA(giftiLabel)));
                if (clusterContainer != NULL) {
                    item->setClusters(clusterContainer->getClustersWithKey(labelKey));
                }
                parentItem->appendRow(item);
                m_labelKeyToLabelSelectionItem[labelKey] = item;
            }
        }
        
        topLevelItems.push_back(parentItem);
        
        text.appendWithNewLine("   These labels not in hierarchy have been added to the group \""
                               + parentItem->text()
                               + "\": ");
        for (const AString& name : buildTreeMissingHierarchyNames) {
            text.appendWithNewLine("      " + name);
        }
    }
    
    if ( ! labelIsParentInHierarchyNames.empty()) {
        text.appendWithNewLine("   Label from label table is in the element hierarchy but element contains children:");
        for (const AString& name : labelIsParentInHierarchyNames) {
            text.appendWithNewLine("      " + name);
        }
    }
    
    if ( ! text.isEmpty()) {
        if (m_logMismatchedLabelsFlag) {
            text.insert(0, (m_fileAndMapName + "\n"));
            CaretLogInfo(text);
        }
    }

    for (LabelSelectionItem* item : topLevelItems) {
        invisibleRootItem()->appendRow(item);
    }
    
    const std::set<int32_t> keysNotInClusters(clusterContainer->getKeysThatAreNotInAnyClusters());
    for (auto& keyItem : m_labelKeyToLabelSelectionItem) {
        const int32_t key(keyItem.first);
        if (keysNotInClusters.find(key) != keysNotInClusters.end()) {
            LabelSelectionItem* labelItem(keyItem.second);
            labelItem->appendToToolTip("This label is not used by any brainordinates");
            if ( ! labelItem->hasChildren()) {
                /*
                 * Will not do anything since not used by any brainordinates
                 * AND it has no children so disable it
                 */
                labelItem->setEnabled(false);
            }
        }
    }
    
    setCheckedStatusOfAllItems(true);
    
    updateCheckedStateOfAllItems();
    
    {
        std::vector<LabelSelectionItem*> topLevelItems(getTopLevelItems());
        for (auto& item : topLevelItems) {
            (void)item->setCenterOfGravityFromChildren();
        }
    }
    
    m_validFlag = true;
}

/**
 * Build the tree from the items in the hierarchy
 * @param hierarchyItem
 *    Hierarchy item and its children are added
 * @param giftiLabelTable
 *    The GIFTI label table
 * @param clusterContainer
 *    The cluster container (may be NULL)
 */
LabelSelectionItem*
LabelSelectionItemModel::buildTree(const CaretHierarchy::Item* hierarchyItem,
                                   const GiftiLabelTable* giftiLabelTable,
                                   const ClusterContainer* clusterContainer)
{
    LabelSelectionItem* itemOut(NULL);
    
    std::array<uint8_t, 4> rgba { 255, 255, 255, 255 };
    int32_t labelKey(-1);
    const GiftiLabel* label(giftiLabelTable->getLabel(hierarchyItem->name));
    if (label != NULL) {
        rgba = getLabelRGBA(label);
        labelKey = label->getKey();
    }

    CaretAssert(hierarchyItem);
    const int32_t numChildren(hierarchyItem->children.size());
    if (numChildren > 0) {
        if (label != NULL) {
            itemOut = new LabelSelectionItem(hierarchyItem->name,
                                             hierarchyItem->id,
                                             labelKey,
                                             rgba);
            if (clusterContainer != NULL) {
                itemOut->setClusters(clusterContainer->getClustersWithKey(labelKey));
            }
        }
        else {
            itemOut = new LabelSelectionItem(hierarchyItem->name,
                                             hierarchyItem->id);
        }
        for (int32_t i = 0; i < numChildren; i++) {
            CaretAssertVectorIndex(hierarchyItem->children, i);
            itemOut->appendRow(buildTree(&hierarchyItem->children[i],
                                         giftiLabelTable,
                                         clusterContainer));
        }
        if (labelKey >= 0) {
            m_labelKeyToLabelSelectionItem[labelKey] = itemOut;
        }
        m_hierarchyParentNames.insert(hierarchyItem->name);
    }
    else {
        AString name(hierarchyItem->name);
        itemOut = new LabelSelectionItem(name,
                                         hierarchyItem->id,
                                         labelKey,
                                         rgba);
        if (clusterContainer != NULL) {
            itemOut->setClusters(clusterContainer->getClustersWithKey(labelKey));
        }

        if (labelKey >= 0) {
            m_labelKeyToLabelSelectionItem[labelKey] = itemOut;
        }
        if (label == NULL) {
            m_buildTreeMissingLabelNames.insert(name);
            itemOut->appendToToolTip("There is no label in the label table for this name");
            if ( ! itemOut->hasChildren()) {
                /*
                 * Will not do anything since there is no label in the label table
                 * AND it has no children so disable it
                 */
                itemOut->setEnabled(false);
            }
        }
    }
    
    return itemOut;
}

/**
 * @return The RGBA color for the label as four bytes.  If the label is NULL,
 * white it returned.
 * @param label
 *    The GIFTI label
 */
std::array<uint8_t, 4>
LabelSelectionItemModel::getLabelRGBA(const GiftiLabel* label) const
{
    std::array<uint8_t, 4> rgba { 255, 255, 255, 255 };
    if (label == NULL) {
        return rgba;
    }

    const std::array<float, 4> rgbaFloat {
        label->getRed(),
        label->getGreen(),
        label->getBlue(),
        label->getAlpha()
    };
    
    for (int i = 0; i < 4; i++) {
        int32_t c(static_cast<int32_t>(rgbaFloat[i] * 255.0));
        if (c > 255) c = 255;
        if (c < 0) c = 0;
        rgba[i] = c;
    }
    
    return rgba;
}

/**
 * @return All descendants that are of the given type
 * @param itemType
 *    Type for matching to QStandardItem::type()
 * @return
 *    Vector containing the matching items
 */
std::vector<LabelSelectionItem*> 
LabelSelectionItemModel::getAllDescendantsOfType(const LabelSelectionItem::ItemType itemType)
{
    std::vector<LabelSelectionItem*> itemsOut;
    
    QStandardItem* rootItem(invisibleRootItem());
    const int32_t numChildren(rootItem->rowCount());
    for (int32_t iRow = 0; iRow < numChildren; iRow++) {
        QStandardItem* childItem(rootItem->child(iRow));
        LabelSelectionItem* labelItem(dynamic_cast<LabelSelectionItem*>(childItem));
        CaretAssert(labelItem);
        std::vector<LabelSelectionItem*> items(labelItem->getThisAndAllDescendantsOfType(itemType));
        itemsOut.insert(itemsOut.end(),
                        items.begin(), items.end());
    }
    
    return itemsOut;
}

/**
 * @return All items at the top level of the hierarchy
 */
std::vector<LabelSelectionItem*>
LabelSelectionItemModel::getTopLevelItems()
{
    std::vector<LabelSelectionItem*> itemsOut;
    
    QStandardItem* rootItem(invisibleRootItem());
    const int32_t numChildren(rootItem->rowCount());
    for (int32_t iRow = 0; iRow < numChildren; iRow++) {
        QStandardItem* childItem(rootItem->child(iRow));
        LabelSelectionItem* labelItem(dynamic_cast<LabelSelectionItem*>(childItem));
        CaretAssert(labelItem);
        itemsOut.push_back(labelItem);
    }
    
    return itemsOut;
}
/**
 * @return All descendants
 */
std::vector<LabelSelectionItem*> 
LabelSelectionItemModel::getAllDescendants()
{
    std::vector<LabelSelectionItem*> itemsOut;
    
    QStandardItem* rootItem(invisibleRootItem());
    const int32_t numChildren(rootItem->rowCount());
    for (int32_t iRow = 0; iRow < numChildren; iRow++) {
        QStandardItem* childItem(rootItem->child(iRow));
        LabelSelectionItem* labelItem(dynamic_cast<LabelSelectionItem*>(childItem));
        CaretAssert(labelItem);
        std::vector<LabelSelectionItem*> items(labelItem->getThisAndAllDescendants());
        itemsOut.insert(itemsOut.end(),
                        items.begin(), items.end());
    }
    
    return itemsOut;
}

/**
 * @return A formatted string showing the hierarchy
 * @param indentation
 *    Indentation for the string
 */
AString
LabelSelectionItemModel::toFormattedString(const AString& indentation) const
{
    AString text;
    
    QStandardItem* rootItem(invisibleRootItem());
    const int32_t numChildren(rootItem->rowCount());
    for (int32_t iRow = 0; iRow < numChildren; iRow++) {
        QStandardItem* childItem(rootItem->child(iRow));
        LabelSelectionItem* labelItem(dynamic_cast<LabelSelectionItem*>(childItem));
        CaretAssert(labelItem);
        text.appendWithNewLine(labelItem->toFormattedString(indentation + "   "));
    }
    
    return text;
}

/**
 * Synchronize selections between this label hiearchy and the list in the label table
 * @param copyToLabelTableFlag
 *   If true, copy selections from this model to the label table,
 *      else copy selections from the label table to this model.
 */
void 
LabelSelectionItemModel::synchronizeSelectionsWithLabelTable(const bool copyToLabelTableFlag)
{
    for (auto& keyItem : m_labelKeyToLabelSelectionItem) {
        const int32_t key(keyItem.first);
        CaretAssert(key >= 0);
        LabelSelectionItem* labelSelectionItem(keyItem.second);
        CaretAssert(labelSelectionItem);
        
        GiftiLabel* label(m_giftiLabelTable->getLabel(key));
        if (label != NULL) {
            GroupAndNameHierarchyItem* gnhn(label->getGroupNameSelectionItem());
            CaretAssert(gnhn);
            if (copyToLabelTableFlag) {
                const bool selectedFlag(labelSelectionItem->checkState() != Qt::Unchecked);
                gnhn->setSelected(m_displayGroup, m_tabIndex, selectedFlag);
            }
            else {
                const Qt::CheckState checkState(gnhn->isSelected(m_displayGroup, m_tabIndex)
                                                ? Qt::Checked
                                                : Qt::Unchecked);
                labelSelectionItem->setCheckState(checkState);
            }
        }
    }
    
    if ( ! copyToLabelTableFlag) {
        updateCheckedStateOfAllItems();
    }
}


/**
 * Save information specific to this type of model to the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    saving the scene.
 *
 * @param instanceName
 *    Name of instance in the scene.
 */
SceneClass*
LabelSelectionItemModel::saveToScene(const SceneAttributes* sceneAttributes,
                                     const AString& instanceName)
{
    SceneClass* sceneClass = new SceneClass(instanceName,
                                            "LabelSelectionItemModel",
                                            1);
    m_sceneAssistant->saveMembers(sceneAttributes,
                                  sceneClass);
    
    /*
     * Save only those labels that are checked
     */
    std::vector<AString> checkedLabelNames;
    const std::vector<LabelSelectionItem*> labelItems(getAllDescendantsOfType(LabelSelectionItem::ItemType::ITEM_LABEL));
    for (const LabelSelectionItem* lsi : labelItems) {
        if (lsi->checkState() == Qt::Checked) {
            checkedLabelNames.push_back(lsi->text());
        }
    }
    
    if ( ! checkedLabelNames.empty()) {
        sceneClass->addStringArray("CheckedLabels",
                                   &checkedLabelNames[0],
                                   checkedLabelNames.size());
    }
    
    // Uncomment if sub-classes must save to scene
    //saveSubClassDataToScene(sceneAttributes,
    //                        sceneClass);
    
    return sceneClass;
}

/**
 * Restore information specific to the type of model from the scene.
 *
 * @param sceneAttributes
 *    Attributes for the scene.  Scenes may be of different types
 *    (full, generic, etc) and the attributes should be checked when
 *    restoring the scene.
 *
 * @param sceneClass
 *     sceneClass from which model specific information is obtained.
 */
void
LabelSelectionItemModel::restoreFromScene(const SceneAttributes* sceneAttributes,
                                          const SceneClass* sceneClass)
{
    if (sceneClass == NULL) {
        return;
    }
    
    m_sceneAssistant->restoreMembers(sceneAttributes,
                                     sceneClass);    
    
    /*
     * Restore checked status of labels
     */
    bool haveCheckedLabelsFlag(false);
    const ScenePrimitiveArray* checkedLabelsArray(sceneClass->getPrimitiveArray("CheckedLabels"));
    if (checkedLabelsArray != NULL) {
        std::set<AString> checkedLabelNames;
        const int32_t numLabels(checkedLabelsArray->getNumberOfArrayElements());
        for (int32_t i = 0; i < numLabels; i++) {
            checkedLabelNames.insert(checkedLabelsArray->stringValue(i));
        }
        
        if ( ! checkedLabelNames.empty()) {
            haveCheckedLabelsFlag = true;
            
            std::vector<LabelSelectionItem*> labelItems(getAllDescendantsOfType(LabelSelectionItem::ItemType::ITEM_LABEL));
            for (LabelSelectionItem* item : labelItems) {
                if (checkedLabelNames.find(item->text()) != checkedLabelNames.end()) {                    item->setCheckState(Qt::Checked);
                }
                else {
                    item->setCheckState(Qt::Unchecked);
                }
            }
        }
    }
    
    if ( ! haveCheckedLabelsFlag) {
        setCheckedStatusOfAllItems(true);
    }
    
    updateCheckedStateOfAllItems();
    
    //Uncomment if sub-classes must restore from scene
    //restoreSubClassDataFromScene(sceneAttributes,
    //                             sceneClass);
}

