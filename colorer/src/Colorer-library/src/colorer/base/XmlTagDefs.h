#ifndef COLORER_XMLTAGDEFS_H
#define COLORER_XMLTAGDEFS_H

/* catalog.xml

<catalog>
  <hrc-sets>
    <location link=""/>
  </hrc-sets>
  <hrd-sets>
    <hrd class="" name="" description="">
      <location link=""/>
    </hrd>
  </hrd-sets>
</catalog>
*/

const XMLCh catTagCatalog[] = u"catalog\0";
const XMLCh catTagHrcSets[] = u"hrc-sets\0";
const XMLCh catTagLocation[] = u"location\0";
const XMLCh catLocationAttrLink[] = u"link\0";
const XMLCh catTagHrdSets[] = u"hrd-sets\0";
const XMLCh catTagHrd[] = u"hrd\0";
const XMLCh catHrdAttrClass[] = u"class\0";
const XMLCh catHrdAttrName[] = u"name\0";
const XMLCh catHrdAttrDescription[] = u"description\0";

/* hrc file

<hrc version="">
  <annotation/>
  <prototype name="" group="" description="">
    <annotation/>
    <location link=""/>
    <filename weight=""></filename>
    <firstline weight=""></firstline>
    <parameters>
      <param name="" value="" description=""/>
    </parameters>
  </prototype>
  <package name="" group="" description="">
    <annotation/>
    <location link=""/>
  </package>
  <type name="">
    <annotation/>
    <import type=""/>
    <region name="" parent="" description=""/>
    <entity name="" value=""/>
    <scheme name="" if="" unless="">
      <annotation/>
      <keywords ignorecase="" region="" priority="" worddiv="">
        <word name="" region=""/>
        <symb name="" region=""/>
      </keywords>
      <regexp match="" region="" priority=""/>
      <block start="" end="" scheme="" priority="" content-priority="" inner-region="" region=""/>
      <inherit scheme="">
        <virtual scheme="" subst-scheme=""/>
      </inherit>
    </scheme>
  </type>
</hrc>
*/

const XMLCh hrcTagHrc[] = u"hrc\0";
const XMLCh hrcHrcAttrVersion[] = u"version\0";
const XMLCh hrcTagAnnotation[] = u"annotation\0";
const XMLCh hrcTagPrototype[] = u"prototype\0";
const XMLCh hrcPrototypeAttrName[] = u"name\0";
const XMLCh hrcPrototypeAttrDescription[] = u"description\0";
const XMLCh hrcPrototypeAttrGroup[] = u"group\0";
const XMLCh hrcTagLocation[] = u"location\0";
const XMLCh hrcLocationAttrLink[] = u"link\0";
const XMLCh hrcTagFilename[] = u"filename\0";
const XMLCh hrcFilenameAttrWeight[] = u"weight\0";
const XMLCh hrcTagFirstline[] = u"firstline\0";
const XMLCh hrcFirstlineAttrWeight[] = u"weight\0";
const XMLCh hrcTagParametrs[] = u"parameters\0";
const XMLCh hrcTagParam[] = u"param\0";
const XMLCh hrcParamAttrName[] = u"name\0";
const XMLCh hrcParamAttrValue[] = u"value\0";
const XMLCh hrcParamAttrDescription[] = u"description\0";
const XMLCh hrcTagPackage[] = u"package\0";
const XMLCh hrcPackageAttrName[] = u"name\0";
const XMLCh hrcPackageAttrDescription[] = u"description\0";
const XMLCh hrcPackageAttrGroup[] = u"group\0";
const XMLCh hrcTagType[] = u"type\0";
const XMLCh hrcTypeAttrName[] = u"name\0";
const XMLCh hrcTagImport[] = u"import\0";
const XMLCh hrcImportAttrType[] = u"type\0";
const XMLCh hrcTagRegion[] = u"region\0";
const XMLCh hrcRegionAttrName[] = u"name\0";
const XMLCh hrcRegionAttrParent[] = u"parent\0";
const XMLCh hrcRegionAttrDescription[] = u"description\0";
const XMLCh hrcTagEntity[] = u"entity\0";
const XMLCh hrcEntityAttrName[] = u"name\0";
const XMLCh hrcEntityAttrValue[] = u"value\0";
const XMLCh hrcTagScheme[] = u"scheme\0";
const XMLCh hrcSchemeAttrName[] = u"name\0";
const XMLCh hrcSchemeAttrIf[] = u"if\0";
const XMLCh hrcSchemeAttrUnless[] = u"unless\0";
const XMLCh hrcTagKeywords[] = u"keywords\0";
const XMLCh hrcKeywordsAttrIgnorecase[] = u"ignorecase\0";
const XMLCh hrcKeywordsAttrPriority[] = u"priority\0";
const XMLCh hrcKeywordsAttrWorddiv[] = u"worddiv\0";
const XMLCh hrcKeywordsAttrRegion[] = u"region\0";
const XMLCh hrcTagWord[] = u"word\0";
const XMLCh hrcWordAttrName[] = u"name\0";
const XMLCh hrcWordAttrRegion[] = u"region\0";
const XMLCh hrcTagSymb[] = u"symb\0";
const XMLCh hrcSymbAttrName[] = u"name\0";
const XMLCh hrcSymbAttrRegion[] = u"region\0";
const XMLCh hrcTagRegexp[] = u"regexp\0";
const XMLCh hrcRegexpAttrMatch[] = u"match\0";
const XMLCh hrcRegexpAttrPriority[] = u"priority\0";
const XMLCh hrcRegexpAttrRegion[] = u"region\0";
const XMLCh hrcTagBlock[] = u"block\0";
const XMLCh hrcBlockAttrStart[] = u"start\0";
const XMLCh hrcBlockAttrEnd[] = u"end\0";
const XMLCh hrcBlockAttrScheme[] = u"scheme\0";
const XMLCh hrcBlockAttrPriority[] = u"priority\0";
const XMLCh hrcBlockAttrContentPriority[] = u"content-priority\0";
const XMLCh hrcBlockAttrInnerRegion[] = u"inner-region\0";
const XMLCh hrcBlockAttrMatch[] = u"match\0";
const XMLCh hrcTagInherit[] = u"inherit\0";
const XMLCh hrcInheritAttrScheme[] = u"scheme\0";
const XMLCh hrcTagVirtual[] = u"virtual\0";
const XMLCh hrcVirtualAttrScheme[] = u"scheme\0";
const XMLCh hrcVirtualAttrSubstScheme[] = u"subst-scheme\0";

/* hrd file
<hrd>
  <assign name="" fore="" back="" style=""/>
  <assign name="" stext="" etext="" sback="" eback=""/>
</hrc>
*/
const XMLCh hrdTagHrd[] = u"hrd\0";
const XMLCh hrdTagAssign[] = u"assign\0";
const XMLCh hrdAssignAttrName[] = u"name\0";
const XMLCh hrdAssignAttrFore[] = u"fore\0";
const XMLCh hrdAssignAttrBack[] = u"back\0";
const XMLCh hrdAssignAttrStyle[] = u"style\0";
const XMLCh hrdAssignAttrSBack[] = u"sback\0";
const XMLCh hrdAssignAttrEBack[] = u"eback\0";
const XMLCh hrdAssignAttrSText[] = u"stext\0";
const XMLCh hrdAssignAttrEText[] = u"etext\0";

/*
 * attributes value
 */
const XMLCh value_low[] = u"low\0";
const XMLCh value_yes[] = u"yes\0";

#endif  // COLORER_XMLTAGDEFS_H
