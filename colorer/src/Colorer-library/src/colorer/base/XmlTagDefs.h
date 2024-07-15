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

XMLCH_LITERAL(catTagCatalog, u"catalog\0");
XMLCH_LITERAL(catTagHrcSets, u"hrc-sets\0");
XMLCH_LITERAL(catTagLocation, u"location\0");
XMLCH_LITERAL(catLocationAttrLink, u"link\0");
XMLCH_LITERAL(catTagHrdSets, u"hrd-sets\0");
XMLCH_LITERAL(catTagHrd, u"hrd\0");
XMLCH_LITERAL(catHrdAttrClass, u"class\0");
XMLCH_LITERAL(catHrdAttrName, u"name\0");
XMLCH_LITERAL(catHrdAttrDescription, u"description\0");

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

XMLCH_LITERAL(hrcTagHrc, u"hrc\0");
XMLCH_LITERAL(hrcHrcAttrVersion, u"version\0");
XMLCH_LITERAL(hrcTagAnnotation, u"annotation\0");
XMLCH_LITERAL(hrcTagPrototype, u"prototype\0");
XMLCH_LITERAL(hrcPrototypeAttrName, u"name\0");
XMLCH_LITERAL(hrcPrototypeAttrDescription, u"description\0");
XMLCH_LITERAL(hrcPrototypeAttrGroup, u"group\0");
XMLCH_LITERAL(hrcTagLocation, u"location\0");
XMLCH_LITERAL(hrcLocationAttrLink, u"link\0");
XMLCH_LITERAL(hrcTagFilename, u"filename\0");
XMLCH_LITERAL(hrcFilenameAttrWeight, u"weight\0");
XMLCH_LITERAL(hrcTagFirstline, u"firstline\0");
XMLCH_LITERAL(hrcFirstlineAttrWeight, u"weight\0");
XMLCH_LITERAL(hrcTagParametrs, u"parameters\0");
XMLCH_LITERAL(hrcTagParam, u"param\0");
XMLCH_LITERAL(hrcParamAttrName, u"name\0");
XMLCH_LITERAL(hrcParamAttrValue, u"value\0");
XMLCH_LITERAL(hrcParamAttrDescription, u"description\0");
XMLCH_LITERAL(hrcTagPackage, u"package\0");
XMLCH_LITERAL(hrcPackageAttrName, u"name\0");
XMLCH_LITERAL(hrcPackageAttrDescription, u"description\0");
XMLCH_LITERAL(hrcPackageAttrGroup, u"group\0");
XMLCH_LITERAL(hrcTagType, u"type\0");
XMLCH_LITERAL(hrcTypeAttrName, u"name\0");
XMLCH_LITERAL(hrcTagImport, u"import\0");
XMLCH_LITERAL(hrcImportAttrType, u"type\0");
XMLCH_LITERAL(hrcTagRegion, u"region\0");
XMLCH_LITERAL(hrcRegionAttrName, u"name\0");
XMLCH_LITERAL(hrcRegionAttrParent, u"parent\0");
XMLCH_LITERAL(hrcRegionAttrDescription, u"description\0");
XMLCH_LITERAL(hrcTagEntity, u"entity\0");
XMLCH_LITERAL(hrcEntityAttrName, u"name\0");
XMLCH_LITERAL(hrcEntityAttrValue, u"value\0");
XMLCH_LITERAL(hrcTagScheme, u"scheme\0");
XMLCH_LITERAL(hrcSchemeAttrName, u"name\0");
XMLCH_LITERAL(hrcSchemeAttrIf, u"if\0");
XMLCH_LITERAL(hrcSchemeAttrUnless, u"unless\0");
XMLCH_LITERAL(hrcTagKeywords, u"keywords\0");
XMLCH_LITERAL(hrcKeywordsAttrIgnorecase, u"ignorecase\0");
XMLCH_LITERAL(hrcKeywordsAttrPriority, u"priority\0");
XMLCH_LITERAL(hrcKeywordsAttrWorddiv, u"worddiv\0");
XMLCH_LITERAL(hrcKeywordsAttrRegion, u"region\0");
XMLCH_LITERAL(hrcTagWord, u"word\0");
XMLCH_LITERAL(hrcWordAttrName, u"name\0");
XMLCH_LITERAL(hrcWordAttrRegion, u"region\0");
XMLCH_LITERAL(hrcTagSymb, u"symb\0");
XMLCH_LITERAL(hrcSymbAttrName, u"name\0");
XMLCH_LITERAL(hrcSymbAttrRegion, u"region\0");
XMLCH_LITERAL(hrcTagRegexp, u"regexp\0");
XMLCH_LITERAL(hrcRegexpAttrMatch, u"match\0");
XMLCH_LITERAL(hrcRegexpAttrPriority, u"priority\0");
XMLCH_LITERAL(hrcRegexpAttrRegion, u"region\0");
XMLCH_LITERAL(hrcTagBlock, u"block\0");
XMLCH_LITERAL(hrcBlockAttrStart, u"start\0");
XMLCH_LITERAL(hrcBlockAttrEnd, u"end\0");
XMLCH_LITERAL(hrcBlockAttrScheme, u"scheme\0");
XMLCH_LITERAL(hrcBlockAttrPriority, u"priority\0");
XMLCH_LITERAL(hrcBlockAttrContentPriority, u"content-priority\0");
XMLCH_LITERAL(hrcBlockAttrInnerRegion, u"inner-region\0");
XMLCH_LITERAL(hrcBlockAttrMatch, u"match\0");
XMLCH_LITERAL(hrcTagInherit, u"inherit\0");
XMLCH_LITERAL(hrcInheritAttrScheme, u"scheme\0");
XMLCH_LITERAL(hrcTagVirtual, u"virtual\0");
XMLCH_LITERAL(hrcVirtualAttrScheme, u"scheme\0");
XMLCH_LITERAL(hrcVirtualAttrSubstScheme, u"subst-scheme\0");

/* hrd file
<hrd>
  <assign name="" fore="" back="" style=""/>
  <assign name="" stext="" etext="" sback="" eback=""/>
</hrc>
*/
XMLCH_LITERAL(hrdTagHrd, u"hrd\0");
XMLCH_LITERAL(hrdTagAssign, u"assign\0");
XMLCH_LITERAL(hrdAssignAttrName, u"name\0");
XMLCH_LITERAL(hrdAssignAttrFore, u"fore\0");
XMLCH_LITERAL(hrdAssignAttrBack, u"back\0");
XMLCH_LITERAL(hrdAssignAttrStyle, u"style\0");
XMLCH_LITERAL(hrdAssignAttrSBack, u"sback\0");
XMLCH_LITERAL(hrdAssignAttrEBack, u"eback\0");
XMLCH_LITERAL(hrdAssignAttrSText, u"stext\0");
XMLCH_LITERAL(hrdAssignAttrEText, u"etext\0");

/*
 * attributes value
 */
XMLCH_LITERAL(value_low, u"low\0");
XMLCH_LITERAL(value_yes, u"yes\0");

#endif  // COLORER_XMLTAGDEFS_H
