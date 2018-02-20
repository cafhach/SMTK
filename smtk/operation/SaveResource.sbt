<?xml version="1.0" encoding="utf-8" ?>
<!-- Description of the model "SaveResource" Operation -->
<SMTK_AttributeSystem Version="3">

  <Definitions>
    <!-- Operation -->
    <include href="smtk/operation/Operation.xml"/>
    <AttDef Type="save resource" Label="Save" BaseType="operation">
      <BriefDecscription>
        Save one or more SMTK resources to disk
      </BriefDecscription>
      <DetailedDecscription>
        Save resources in SMTK's native JSON format.
        Resources are written to their existing location unless
        the "filename" item is enabled and set to a valid value.
      </DetailedDecscription>
      <ItemDefinitions>
        <Resource Name="resource" NumberOfRequiredValues="1" Extensible="true">
          <BriefDescription>The resource(s) to save.</BriefDescription>
        </Resource>
        <File Name="filename" NumberOfRequiredValues="1" Extensible="true" Optional="true"
          FileFilters="SMTK Resource (*.smtk)" Label="SMTK Resource File Name " ShouldExist="false">
          <BriefDescription>The destination filename for "save as" behavior.</BriefDescription>
        </File>
      </ItemDefinitions>
    </AttDef>

    <!-- Result -->
    <include href="smtk/operation/Result.xml"/>
    <AttDef Type="result(save resource)" BaseType="result">
    </AttDef>
  </Definitions>

</SMTK_AttributeSystem>