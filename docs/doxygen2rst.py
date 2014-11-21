from __future__ import print_function
import re, sys, os, time, glob, errno, tempfile, binascii, subprocess, shutil
from lxml import etree
from optparse import OptionParser
import textwrap
import string

VERSION = '0.1'
__all__ = ['DoxyGen2RST']
LINE_BREAKER = "\n"
MAX_COLUMN = 80

def is_valid_uuid(uuid_string):
    uuid4hex = re.compile('[0-9a-f]{32}\Z', re.I)
    return uuid4hex.match(uuid_string) != None

def get_page(refid):
    fields = refid.split("_")
    if(is_valid_uuid(fields[-1][-32:])):
        return ["_".join(fields[0:-1]), fields[-1]]
    return [refid, None]

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise

def _glob(path, *exts):
    path = os.path.join(path, "*") if os.path.isdir(path) else path + "*"
    return [f for files in [glob.glob(path + ext) for ext in exts] for f in files]

class DoxyGen2RST(object):
    """
    Customize the Doxygen XML output into RST format, then it can
    be translated into all formats with the unified user interface.
    The Doxygen output itself is too verbose and not hard to be
    organized for a good documentation.
    """

    def __init__(self,
                 src,
                 dst,
                 missing_filename = "missing.rst",
                 is_github = False,
                 enable_uml = True,
                 github_ext = ""):
        self.doxy_output_dir = os.path.join(src, "_doxygen", "xml")
        self.output_dir = dst
        self.rst_dir = src
        self.enable_uml = enable_uml
        mkdir_p(dst)
        self.is_github = is_github
        if(is_github):
            self.page_ext = github_ext
            self.anchor_prefix = "wiki-"
        else:
            self.anchor_prefix = ""
            self.page_ext = ".html"
        self.filter = ["*.rst", "*.rest"]
        self.re_doxy = "<doxygen2rst\s(\S*)=(\S*)>(.*?)</doxygen2rst>"
        self.index_root = etree.parse(os.path.join(self.doxy_output_dir, "index.xml")).getroot()
        self.references = {}
        self.missed_types_structs = {}
        self.name_refid_map = {}
        self.build_references()
        self.page_references = {}
        self.missing_filename = missing_filename
        self.temp_uml_path = os.path.join(tempfile.gettempdir(), "uml_" + binascii.b2a_hex(os.urandom(15)))
        if os.path.exists(self.temp_uml_path):
            shutil.rmtree(self.temp_uml_path)
        os.mkdir(self.temp_uml_path)

    def _find_ref_id(self, kind, name):
        #print("_find_ref_id, %s - %s" %(kind, name))
        if(kind == "function"):
            for comp in self.index_root.iter("member"):
                if(comp.attrib["kind"].lower() == kind.lower() and
                   comp.findtext("name").lower() == name.lower()):
                    return (comp.attrib["refid"])
            pass
        else:
            for comp in self.index_root.iter("compound"):
                if(comp.attrib["kind"].lower() == kind.lower() and
                   comp.findtext("name").lower() == name.lower()):
                   return comp.attrib["refid"]
        return None

    def strip_title_ref(self, text):
        table = string.maketrans("","")
        retstr = text.translate(table, string.punctuation)
        words = retstr.split()
        retstr = "-".join(words)
        return retstr.lower()

    def build_references(self):
        for file in _glob(self.rst_dir, *self.filter):
            filename = os.path.basename(file)
            fin = open(file,'r')
            content = fin.read()
            it = re.finditer(self.re_doxy, content, re.DOTALL)
            for m in it:
                ref_id = self._find_ref_id(m.groups()[0], m.groups()[1])
                if(ref_id is None):
                    #print("Reference is NOT found for: %s=%s" % (m.groups()[0], m.groups()[1]))
                    continue
                page_name = os.path.splitext(filename)[0]
                title_ref = self.strip_title_ref(m.groups()[2])
                self.references[ref_id] = [m.groups()[0], m.groups()[1], page_name, filename, title_ref]
                self.name_refid_map[m.groups()[1]] = ref_id
            fin.close()
        #print(self.references)

    def call_plantuml(self):
        if(not self.enable_uml):
            return

        java_bin = os.path.join(os.environ['JAVA_HOME'], "bin", "java")
        output_path = os.path.abspath(os.path.join(self.output_dir, "images"))
        cmds = ["\"" + java_bin + "\"", "-jar", "plantuml.jar", self.temp_uml_path + "/", "-o", output_path]
        print(" ".join(cmds))
        os.system(" ".join(cmds))
        shutil.rmtree(self.temp_uml_path)

    def _build_uml(self, uml_name, content):
        uml_path = os.path.join(self.temp_uml_path, uml_name + ".txt")
        fuml = open(uml_path, "w+")
        fuml.write("@startuml\n")
        fuml.write(content)
        fuml.write("\n@enduml\n")
        fuml.close()
        return ".. image:: images/" + uml_name + ".png" + LINE_BREAKER

    def _build(self, m):
        retstr = ""
        if(m.groups()[0] == "uml"):
            retstr = self._build_uml(m.groups()[1], m.groups()[2])
        elif(m.groups()[0] == "link"):
            link = m.groups()[1] + self.page_ext
            retstr = ("`%s <%s>`_" % (m.groups()[2], link))
        else:
            if(m.groups()[0] != "function"):
                retstr +=  self._build_title(m.groups()[2])
            retstr += self.convert_doxy(m.groups()[0], m.groups()[1])

        return retstr

    def generate(self):
        for file in _glob(self.rst_dir, *self.filter):
            filename = os.path.basename(file)
            fin = open(file,'r')
            input_txt = fin.read()
            fin.close()

            output_txt = re.sub(self.re_doxy, self._build, input_txt, 0, re.DOTALL)
            output_txt  += self._build_page_ref_notes()

            fout = open(os.path.join(self.output_dir, filename), 'w+')
            fout.write(output_txt)
            fout.close()
            #print("%s --- %s" %( file, os.path.join(self.output_dir, filename)))

        self._build_missed_types_and_structs()
        self.call_plantuml()

    def make_para_title(self, title,  indent = 4):
        retstr = LINE_BREAKER
        if(title):
            retstr += "".ljust(indent, " ") + "| **" + title + "**" +  LINE_BREAKER
        return retstr

    def _build_title(self, title, flag = '=', ref = None):
        retstr = LINE_BREAKER
        if(ref):
            retstr += ".. _ref-" + ref + ":" + LINE_BREAKER + LINE_BREAKER
        retstr += title + LINE_BREAKER
        retstr += "".ljust(20, flag) + LINE_BREAKER
        retstr += LINE_BREAKER
        return retstr

    def _build_ref(self, node):
        text = node.text.strip()
        retstr = ""
        target = '`' + text + '`'
        retstr += target + "_ "
        if target in self.page_references:
            reflink = self.page_references[target]
            print("Link already added: %s == %s" % (reflink[0], node.attrib["refid"]))
            assert(reflink[0] == node.attrib["refid"])
            pass
        else:
            self.page_references[target] = (node.attrib["refid"], node.attrib["kindref"], text)

        return retstr

    def _build_code_block(self, node):
        retstr = "::" + LINE_BREAKER + LINE_BREAKER
        for codeline in node.iter("codeline"):
            retstr += "  "
            for phrases in codeline.iter("highlight"):
                if(phrases.text):
                    retstr += phrases.text.strip()
                for child in phrases:
                    if(child.text):
                        retstr += child.text.strip()
                    if(child.tag == "sp"):
                        retstr += " "
                    if(child.tag == "ref" and child.text):
                        #escape the reference in the code block
                        retstr += "" # self._build_ref(child)
                    if(child.tail):
                        retstr += child.tail.strip()
            retstr += LINE_BREAKER
        return retstr

    def _build_itemlist(self, node):
        retstr = ""
        for para in node:
            if(para.tag != "para"):
                continue
            if(para.text):
                retstr += para.text.strip()
            for child in para:
                if(child.tag == "ref" and child.text):
                    retstr += self._build_ref(child)
                if(child.tail):
                    retstr += child.tail.strip()

        return retstr

    def _build_itemizedlist(self, node):
        retstr = LINE_BREAKER
        if(node == None):
            return ""
        for item in node:
            if(item.tag != "listitem"):
                continue
            retstr += "    - " + self._build_itemlist(item)
            retstr += LINE_BREAKER
        return retstr

    def _build_verbatim(self, node):
        retstr = LINE_BREAKER
        if(node.text):
            lines = node.text.splitlines()
            print(lines[0])
            m = re.search("{plantuml}\s(\S*)", lines[0])
            if(m):
                uml_name = "uml_" + m.groups()[0]
                retstr += self._build_uml(uml_name, "\n".join(lines[1:]))
            else:
                retstr += "::" + LINE_BREAKER + LINE_BREAKER
                retstr += node.text

        return retstr

    def _build_para(self, para):
        retstr = ""
        no_new_line = False
        if(para.text):
            retstr += textwrap.fill(para.text.strip(), MAX_COLUMN) + LINE_BREAKER + LINE_BREAKER
        for child in para:
            no_new_line = False
            if(child.tag == "simplesect"):
                for child_para in child:
                    if(child.attrib["kind"] == "return"):
                        return_str = self._build_para(child_para)
                        retstr += "".ljust(4, " ") + "| Return:" + LINE_BREAKER
                        for line in return_str.splitlines():
                            retstr += "".ljust(4, " ") + "| " + line + LINE_BREAKER
                    elif(child_para.tag == "title" and child_para.text):
                        lf.make_para_title(child_para.text.strip(), 4)
                    elif(child_para.tag == "para"): #for @see
                        retstr += self._build_para(child_para)
                    elif(child_para.text):
                        retstr += "".ljust(4, " ") + "| " + child_para.text.strip() + LINE_BREAKER
            if(child.tag == "preformatted"):
                retstr += "::" + LINE_BREAKER + LINE_BREAKER
                if(child.text):
                    for line in child.text.splitlines():
                        retstr += "  " + line + LINE_BREAKER
            if(child.tag == "ref" and child.text):
                retstr = retstr.rstrip('\n')
                retstr += " " + self._build_ref(child)
                no_new_line = True
            if(child.tag == "programlisting"):
                retstr += self._build_code_block(child)
            if(child.tag == "itemizedlist"):
                retstr += self._build_itemizedlist(child)
            if(child.tag == "verbatim"):
                retstr += self._build_verbatim(child)
            if(not no_new_line):
                retstr += LINE_BREAKER
            if(child.tail):
                retstr += textwrap.fill(child.tail.strip(), MAX_COLUMN) + LINE_BREAKER + LINE_BREAKER
        return retstr

    def get_text(self, node):
        retstr = ""
        if(node == None):
            return ""
        for para in node:
            if(para.tag != "para"):
                continue
            retstr += self._build_para(para)

        return retstr

    def _find_text_ref(self, node):
        retstr = ""
        if(node.text):
            retstr += node.text.strip()
        for child in node:
            if(child.tag == "ref"):
                retstr += " " + self._build_ref(child) + " "
            if(child.tail):
                retstr += child.tail.strip()
        return retstr

    def _build_row_breaker(self, columns):
        retstr = "+"
        for column in columns:
            retstr += "".ljust(column, "-") + "+"
        return retstr + LINE_BREAKER

    def _wrap_cell(self, text, length = 30):
        newlines = []
        for line in text.splitlines():
            newlines.extend(textwrap.wrap(line, length))
        return newlines

    def _build_row(self, row, columns):
        retstr = ""
        row_lines = []
        max_line = 0
        for i in range(3):
            row_lines.append(row[i].splitlines())
            if(max_line < len(row_lines[i])):
                max_line = len(row_lines[i])

        for i in range(max_line):
            for j in range(3):
                retstr += "|"
                if(len(row_lines[j]) > i):
                    retstr += row_lines[j][i]
                    retstr += "".ljust(columns[j] - len(row_lines[j][i]), " ")
                else:
                    retstr += "".ljust(columns[j], " ")
            retstr += "|" + LINE_BREAKER
        return retstr

    def _build_table(self, rows):
        retstr = ""
        columns = [0, 0, 0]
        for row in rows:
            for i in range(3):
                for rowline in row[i].splitlines():
                    if(columns[i] < len(rowline) + 2):
                        columns[i] = len(rowline) + 2

        #columns[0] = 40 if(columns[0] > 40) else columns[0]
        #columns[1] = 40 if(columns[1] > 40) else columns[1]
        #columns[2] = MAX_COLUMN - columns[0] - columns[1]

        retstr += self._build_row_breaker(columns)
        for row in rows:
            retstr += self._build_row(row, columns)
            retstr += self._build_row_breaker(columns)
        return retstr;

    def build_param_list(self, params, paramdescs):
        retstr = ""
        param_descriptions = []
        for desc in paramdescs:
            param_descriptions.append(desc)

        rows = []
        rows.append(("Name", "Type", "Descritpion"))
        for param in params:
            declname = param.findtext("declname")
            paramdesc = None
            for desc in param_descriptions:
                paramname = desc.findtext("parameternamelist/parametername")
                if(paramname.lower() == declname.lower()):
                    paramdesc = desc.find("parameterdescription")
                    break
            decltype = self._find_text_ref(param.find("type"))
            rows.append((declname, decltype, self.get_text(paramdesc)))

        if(len(rows) > 1):
            retstr += self._build_table(rows)
        return retstr

    def _build_enum(self, member):
        enum_id = member.attrib["id"]
        file, tag = get_page(enum_id)
        retstr = self._build_title(member.findtext("name"), ref = tag)
        detail_node = self.get_desc_node(member)
        if(detail_node is not None):
            retstr += LINE_BREAKER
            retstr += self.get_text(detail_node)

        rows = []
        rows.append(("Name", "Initializer", "Descritpion"))
        for enumvalue in member.iter("enumvalue"):
            name = enumvalue.findtext("name")
            initializer = enumvalue.findtext("initializer")
            if(not initializer):
                initializer = ""
            desc = self.get_text(enumvalue.find("briefdescription"))
            desc += self.get_text(enumvalue.find("detaileddescription"))
            if(not desc):
                desc = ""
            rows.append((name, initializer, desc))

        if(len(rows) > 1):
            retstr += self._build_table(rows)
        return retstr


    def _build_struct(self, node):
        retstr = ""
        detail_node = self.get_desc_node(node)
        if(detail_node is not None):
            retstr += self.get_text(detail_node) + LINE_BREAKER
        rows = []
        rows.append(("Name", "Type", "Descritpion"))
        for member in node.iter("memberdef"):
            if(member.attrib["kind"] == "variable"):
                name = member.findtext("name")
                type = self._find_text_ref(member.find("type"))
                desc = self.get_text(member.find("briefdescription"))
                desc += self.get_text(member.find("detaileddescription"))
                desc += self.get_text(member.find("inbodydescription"))
                if(not desc):
                    desc = ""
                rows.append((name, type, desc))

        if(len(rows) > 1):
            retstr += self._build_table(rows)
        return retstr

    def _build_class(self, node):
        retstr = ""

        for member in node.iter("memberdef"):
            if(member.attrib["kind"] == "function"):
                retstr += self.build_function(member)
        return retstr

    def get_desc_node(self, member):
        detail_node = member.find("detaileddescription")
        brief_node = member.find("briefdescription")
        detail_txt = ""
        if(detail_node == None and brief_node == None):
            return None

        if(detail_node is not None):
            detail_txt = detail_node.findtext("para")

        if(not detail_txt and brief_node != None):
            detail_txt = brief_node.findtext("para")
            detail_node = brief_node

        return detail_node

    def build_function(self, member):
        retstr = ""

        desc_node = self.get_desc_node(member)
        if(desc_node is None):
            return ""
        detail_txt = desc_node.findtext("para")
        if(not detail_txt or detail_txt.strip() == "{ignore}"):
            return ""

        func_id = member.attrib["id"]
        page_id, ref_id = get_page(func_id)
        retstr += self._build_title(member.findtext("name"), '-', ref = ref_id)
        retstr += self.get_text(desc_node)
        retstr += LINE_BREAKER
        detail_node = member.find("detaileddescription")
        if(desc_node != detail_node):
            retstr += self.get_text(detail_node)
        retstr += self.build_param_list(member.iter("param"), detail_node.iter("parameteritem"))
        return retstr

    def _build_missed_types_and_structs(self):
        fout = open(os.path.join(self.output_dir, self.missing_filename), 'w+')
        fout.write(".. contents:: " + LINE_BREAKER)
        fout.write("    :local:"  + LINE_BREAKER)
        fout.write("    :depth: 2" + LINE_BREAKER + LINE_BREAKER)

        footnote = ""
        while (len(self.missed_types_structs) > 0):
            for key, value in self.missed_types_structs.iteritems():
                fout.write(self.covert_item(value[0], key, value[1]))
                #print(value)
            self.missed_types_structs = {}
            footnote += self._build_page_ref_notes()

        fout.write(footnote)

        fout.close()

    def _build_page_ref_notes(self):
        retstr = LINE_BREAKER
        #TODO
        for key, value in self.page_references.iteritems():
            page, tag = get_page(value[0])
            m = re.search("_8h_", page)
            if(m):
                continue;

            rstname = None
            anchor = value[2].lower()
            if not page in self.references:
                self.missed_types_structs[value[0]] = (page, tag)
                rstname = os.path.splitext(self.missing_filename)[0]
            else:
                rstname = self.references[page][2]
                anchor = self.references[page][4]
            #if(tag and not self.is_github):
            #    anchor = self.anchor_prefix + "ref-" + tag
            retstr += ".. _" + key + ": " + rstname + self.page_ext + "#" + anchor
            retstr += LINE_BREAKER + LINE_BREAKER
        self.page_references = {}
        return retstr

    def _build_item_by_id(self, node, id):
        retstr = ""
        for member in node.iter("memberdef"):
            if(member.attrib["id"] != id):
                continue
            if(member.attrib["kind"] == "enum"):
                retstr += self._build_enum(member)
        return retstr

    def covert_item(self, compound, id, tag):
        xml_path = os.path.join(self.doxy_output_dir, "%s.xml" % compound)
        print("covert_item: id=%s, name=%s" % (id, xml_path))
        obj_root = etree.parse(xml_path).getroot()
        retstr = ""
        compound = obj_root.find("compounddef")
        compound_kind = compound.attrib["kind"]
        if(not tag):
            retstr += self._build_title(compound.findtext("compoundname"))
            if(compound_kind == "class"):
                retstr += self._build_class(compound)
            elif(compound_kind == "struct"):
                retstr += self._build_struct(compound)
        else:
            retstr += self._build_item_by_id(compound, id)

        return retstr

    def _build_page(self, compound):
        retstr = ""
        retstr += self.get_text(compound.find("detaileddescription"))
        return retstr

    def _build_file(self, compound, type, ref_id, name):
        retstr = ""
        for member in compound.iter("memberdef"):
            if(member.attrib["kind"] == "function" and member.attrib["id"] == ref_id):
                retstr += self.build_function(member)
        return retstr

    def convert_doxy(self, type, name):
        #print(name)
        file = ref_id = self.name_refid_map[name]
        dst_kind = type
        if(type == "function"):
            file, tag = get_page(ref_id)
            dst_kind = "file"
        xml_path = os.path.join(self.doxy_output_dir, "%s.xml" % file)
        print("convert_doxy: type=%s, name=%s" % (type, xml_path))
        obj_root = etree.parse(xml_path).getroot()
        compound = obj_root.find("compounddef")
        compound_kind = compound.attrib["kind"]
        assert(dst_kind == compound_kind)
        retstr = ""
        if(compound_kind == "class"):
            retstr += self._build_class(compound)
        elif(compound_kind == "struct"):
            retstr += self._build_struct(compound)
        elif(compound_kind == "page"):
            retstr += self._build_page(compound)
        elif(compound_kind == "group"):
            retstr += self._build_page(compound)
        elif(compound_kind == "file"):
            retstr += self._build_file(compound, type, ref_id, name)
        return retstr


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("-g", "--github", action="store_true", help="Render the link in format of github wiki.")
    parser.add_argument("-e", "--ext", default="", help="extension for github wiki")
    parser.add_argument("-i", "--input", default="doxygen", help="Input file path of doxygen output and source rst file.")
    parser.add_argument("-o", "--output", default="wikipage", help="Output converted restructured text files to path.")
    parser.add_argument("-s", "--struct", default="TypesAndStructures.rest", help="Output of auto generated enum and structures.")
    parser.add_argument("-u", "--uml", action="store_true", help="Enable UML, you need to download plantuml.jar from Plantuml and put it to here. http://plantuml.sourceforge.net/")

    args = parser.parse_args()
    ext = ""
    if(len(args.ext) > 0):
        ext = ("." + args.ext)
    agent = DoxyGen2RST(args.input,
                        args.output,
                        args.struct,
                        is_github = True,
                        enable_uml = args.uml,
                        github_ext = ext)
    agent.generate()
