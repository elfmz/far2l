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

UNICODE_LITERAL(catTagCatalog, u"catalog")
UNICODE_LITERAL(catTagHrcSets, u"hrc-sets")
UNICODE_LITERAL(catTagLocation, u"location")
UNICODE_LITERAL(catLocationAttrLink, u"link")
UNICODE_LITERAL(catTagHrdSets, u"hrd-sets")
UNICODE_LITERAL(catTagHrd, u"hrd")
UNICODE_LITERAL(catHrdAttrClass, u"class")
UNICODE_LITERAL(catHrdAttrName, u"name")
UNICODE_LITERAL(catHrdAttrDescription, u"description")

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
  <package name="" group="" description="" global="">
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

UNICODE_LITERAL(hrcTagHrc, u"hrc")
UNICODE_LITERAL(hrcHrcAttrVersion, u"version")
UNICODE_LITERAL(hrcTagAnnotation, u"annotation")
UNICODE_LITERAL(hrcTagPrototype, u"prototype")
UNICODE_LITERAL(hrcPrototypeAttrName, u"name")
UNICODE_LITERAL(hrcPrototypeAttrDescription, u"description")
UNICODE_LITERAL(hrcPrototypeAttrGroup, u"group")
UNICODE_LITERAL(hrcTagLocation, u"location")
UNICODE_LITERAL(hrcLocationAttrLink, u"link")
UNICODE_LITERAL(hrcTagFilename, u"filename")
UNICODE_LITERAL(hrcFilenameAttrWeight, u"weight")
UNICODE_LITERAL(hrcTagFirstline, u"firstline")
UNICODE_LITERAL(hrcFirstlineAttrWeight, u"weight")
UNICODE_LITERAL(hrcTagParametrs, u"parameters")
UNICODE_LITERAL(hrcTagParam, u"param")
UNICODE_LITERAL(hrcParamAttrName, u"name")
UNICODE_LITERAL(hrcParamAttrValue, u"value")
UNICODE_LITERAL(hrcParamAttrDescription, u"description")
UNICODE_LITERAL(hrcTagPackage, u"package")
UNICODE_LITERAL(hrcPackageAttrName, u"name")
UNICODE_LITERAL(hrcPackageAttrDescription, u"description")
UNICODE_LITERAL(hrcPackageAttrGroup, u"group")
UNICODE_LITERAL(hrcPackageAttrGlobal, u"global")
UNICODE_LITERAL(hrcTagType, u"type")
UNICODE_LITERAL(hrcTypeAttrName, u"name")
UNICODE_LITERAL(hrcTagImport, u"import")
UNICODE_LITERAL(hrcImportAttrType, u"type")
UNICODE_LITERAL(hrcTagRegion, u"region")
UNICODE_LITERAL(hrcRegionAttrName, u"name")
UNICODE_LITERAL(hrcRegionAttrParent, u"parent")
UNICODE_LITERAL(hrcRegionAttrDescription, u"description")
UNICODE_LITERAL(hrcTagEntity, u"entity")
UNICODE_LITERAL(hrcEntityAttrName, u"name")
UNICODE_LITERAL(hrcEntityAttrValue, u"value")
UNICODE_LITERAL(hrcTagScheme, u"scheme")
UNICODE_LITERAL(hrcSchemeAttrName, u"name")
UNICODE_LITERAL(hrcSchemeAttrIf, u"if")
UNICODE_LITERAL(hrcSchemeAttrUnless, u"unless")
UNICODE_LITERAL(hrcTagKeywords, u"keywords")
UNICODE_LITERAL(hrcKeywordsAttrIgnorecase, u"ignorecase")
UNICODE_LITERAL(hrcKeywordsAttrPriority, u"priority")
UNICODE_LITERAL(hrcKeywordsAttrWorddiv, u"worddiv")
UNICODE_LITERAL(hrcKeywordsAttrRegion, u"region")
UNICODE_LITERAL(hrcTagWord, u"word")
UNICODE_LITERAL(hrcWordAttrName, u"name")
UNICODE_LITERAL(hrcWordAttrRegion, u"region")
UNICODE_LITERAL(hrcTagSymb, u"symb")
UNICODE_LITERAL(hrcSymbAttrName, u"name")
UNICODE_LITERAL(hrcSymbAttrRegion, u"region")
UNICODE_LITERAL(hrcTagRegexp, u"regexp")
UNICODE_LITERAL(hrcRegexpAttrMatch, u"match")
UNICODE_LITERAL(hrcRegexpAttrPriority, u"priority")
UNICODE_LITERAL(hrcRegexpAttrRegion, u"region")
UNICODE_LITERAL(hrcTagBlock, u"block")
UNICODE_LITERAL(hrcBlockAttrStart, u"start")
UNICODE_LITERAL(hrcBlockAttrEnd, u"end")
UNICODE_LITERAL(hrcBlockAttrScheme, u"scheme")
UNICODE_LITERAL(hrcBlockAttrPriority, u"priority")
UNICODE_LITERAL(hrcBlockAttrContentPriority, u"content-priority")
UNICODE_LITERAL(hrcBlockAttrInnerRegion, u"inner-region")
UNICODE_LITERAL(hrcBlockAttrMatch, u"match")
UNICODE_LITERAL(hrcTagInherit, u"inherit")
UNICODE_LITERAL(hrcInheritAttrScheme, u"scheme")
UNICODE_LITERAL(hrcTagVirtual, u"virtual")
UNICODE_LITERAL(hrcVirtualAttrScheme, u"scheme")
UNICODE_LITERAL(hrcVirtualAttrSubstScheme, u"subst-scheme")

/* hrd file
<hrd>
  <assign name="" fore="" back="" style=""/>
  <assign name="" stext="" etext="" sback="" eback=""/>
</hrc>
*/
UNICODE_LITERAL(hrdTagHrd, u"hrd")
UNICODE_LITERAL(hrdTagAssign, u"assign")
UNICODE_LITERAL(hrdAssignAttrName, u"name")
UNICODE_LITERAL(hrdAssignAttrFore, u"fore")
UNICODE_LITERAL(hrdAssignAttrBack, u"back")
UNICODE_LITERAL(hrdAssignAttrStyle, u"style")
UNICODE_LITERAL(hrdAssignAttrSBack, u"sback")
UNICODE_LITERAL(hrdAssignAttrEBack, u"eback")
UNICODE_LITERAL(hrdAssignAttrSText, u"stext")
UNICODE_LITERAL(hrdAssignAttrEText, u"etext")

/*
 * attributes value
 */
UNICODE_LITERAL(value_low, u"low")
UNICODE_LITERAL(value_yes, u"yes")
UNICODE_LITERAL(value_no, u"no")

#endif  // COLORER_XMLTAGDEFS_H
