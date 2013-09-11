<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  xmlns="http://www.w3.org/1999/xhtml">
    <xsl:output method="xml" version="1.0" indent="yes" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" >        
    </xsl:output>
    <xsl:template match="@*|node()">
        <xsl:copy>
            <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
    </xsl:template>
    
    <xsl:template match='html'>
        <xsl:element name = "html" namespace="http://www.w3.org/1999/xhtml">
            <xsl:attribute name="lang">en</xsl:attribute>
            <xsl:attribute name="xml:lang">en</xsl:attribute>
            <xsl:apply-templates select="@*|node()"/>
        </xsl:element>
    </xsl:template>
    
    
   <xsl:template match = "head">
       <xsl:element   name = "head"> 
           <xsl:element name = "meta">
               <xsl:attribute name="http-equiv">Content-Type</xsl:attribute>
               <xsl:attribute name="content">text/html, charset=utf-8</xsl:attribute>
           </xsl:element>
           <xsl:element name = "meta">
               <xsl:attribute name="name">ocr-system</xsl:attribute>
               <xsl:attribute name="content">tesseract 3.02.02</xsl:attribute>
           </xsl:element>
           <xsl:element name="meta">
               <xsl:attribute name="name">ocr-capabilities</xsl:attribute>    
               <xsl:attribute name="content">ocr_page ocr_carea ocr_par ocr_line ocrx_word</xsl:attribute>    
           </xsl:element>
           <xsl:element name = "title"><xsl:text> </xsl:text></xsl:element>
       </xsl:element>
   </xsl:template>
    
    <xsl:template match="body">
        <xsl:element name = "body">
            <xsl:apply-templates select="@*|node()"/>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match = "div[@class='ocr_page']">
        <xsl:variable name = 'Title'><xsl:value-of select="./@title" disable-output-escaping="yes"></xsl:value-of></xsl:variable>
        <xsl:variable name = "newTitle"><xsl:value-of select = "concat($Title,'; ppageno 0')" ></xsl:value-of></xsl:variable>
        <div>
            <xsl:attribute name="class"><xsl:value-of select="@class"/></xsl:attribute>
          <xsl:attribute name="id">
                <xsl:value-of select="@id"/>
            </xsl:attribute>
            <xsl:attribute name='title'><xsl:value-of select="$newTitle"/></xsl:attribute>
            <xsl:apply-templates/>
        </div>
        
    </xsl:template>
    
    <xsl:template match="div[@class='ocr_carea']">
        <xsl:element name = 'div'>
            <xsl:attribute name='class'>ocr_carea</xsl:attribute>
            <xsl:attribute name='id'><xsl:value-of select="@id"></xsl:value-of></xsl:attribute>
            <xsl:attribute name='title'><xsl:value-of select="@title"></xsl:value-of></xsl:attribute>
            <xsl:apply-templates select="@*|node()"/>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="p">
        <xsl:element name = 'p'>
            <xsl:attribute name='class'>ocr_carea</xsl:attribute>
            <!--<xsl:attribute name='id'><xsl:value-of select="@id"></xsl:value-of></xsl:attribute>
            <xsl:attribute name='title'><xsl:value-of select="@title"></xsl:value-of></xsl:attribute>-->
            <xsl:apply-templates select="@*|node()"/>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match="span[@class='ocr_line']">
        <xsl:element name = 'span'>
            <xsl:attribute name='class'>ocr_line</xsl:attribute>
            <xsl:attribute name='id'><xsl:value-of select="@id"></xsl:value-of></xsl:attribute>
            <xsl:attribute name='title'><xsl:value-of select="@title"></xsl:value-of></xsl:attribute>
            <xsl:apply-templates select="@*|node()"/>
        </xsl:element>
    </xsl:template>
    
    <xsl:template match = "span[@class='ocr_word']">        
        <xsl:if test=". != ''"><!-- we don't want empty words-->
        <xsl:element   name = "span">
            <xsl:attribute name="class">ocrx_word</xsl:attribute>
            <xsl:attribute name="id">
                <xsl:value-of select="@id"/>
            </xsl:attribute>
            <xsl:attribute name='title'><xsl:value-of select='@title' /></xsl:attribute>
            <xsl:value-of select='.'/>
        </xsl:element>
        </xsl:if>
    </xsl:template>
    
        <xsl:template match = "span[@class='ocrx_word']">        
            <!-- do nothing as we have already created ocrx_word from ocr_word -->       
    </xsl:template>
    

</xsl:stylesheet>