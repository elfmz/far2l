//
//  Copyright (c) Cail Lomecb (Igor Ruskih) 1999-2001 <ruiv@uic.nnov.ru>
//  You can use, modify, distribute this code or any other part
//  of this program in sources or in binaries only according
//  to License (see /doc/license.txt for more information).
//

// creates object tree structure on html/xml files

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <utils.h>
#include <windows.h>
#include <fcntl.h>
#include <ScopeHelpers.h>

#ifdef USE_CREGEXP
// you can disable clocale class - but import string services
#include<regexp/clocale.h>
#endif
#include<sgml/sgml.h>
#include<sgml/tools.cpp>

CSgmlEl::CSgmlEl()
{
  eparent= 0;
  enext  = 0;
  eprev  = 0;
  echild = 0;
  chnum  = 0;
  type   = EBASEEL;

  name[0] = 0;
  content= 0;
  contentsz = 0;
  parnum = 0;
}

CSgmlEl::~CSgmlEl()
{
	if ((type == EBASEEL) && enext)
		enext->destroylevel();
	if (echild)
		echild->destroylevel();
	// if (name) delete[] name;
	if (content)
		delete[] content;
	for (int i = 0; i < parnum; i++)
	{
		if (params[i][0])
			delete params[i][0];
		if (params[i][1])
			delete params[i][1];
	}
}

PSgmlEl CSgmlEl::createnew(ElType type, PSgmlEl parent, PSgmlEl after)
{
	PSgmlEl El = new CSgmlEl;
	El->type = type;
	if (parent)
	{
		El->enext = parent->echild;
		El->eparent = parent;
		if (parent->echild)
			parent->echild->eprev = El;
		parent->echild = El;
		parent->chnum++;
		parent->type = EBLOCKEDEL;
	}
	else if (after)
		after->insert(El);
	return El;
}

bool CSgmlEl::init()
{
	return true;
}

bool CSgmlEl::readfile(std::string &path, std::wstring &content)
{
	FDScope fd(path.c_str(), O_RDONLY | O_CLOEXEC);
	if (!fd.Valid())
		return false;

	struct stat s{};
	if (fstat(fd, &s) == -1 || s.st_size < 2)
		return false;

	std::vector<char> buf(s.st_size);
	if (ReadAll(fd, &buf[0], buf.size()) != buf.size())
		return false;

	if (buf.size() >= 3 && memcmp(&buf[0], "\xEF\xBB\xBF", 3) == 0) {
		MB2Wide(&buf[3], buf.size() - 3, content);

	} else {
		MB2Wide(&buf[0], buf.size(), content);
	}
//fprintf(stderr, "CSgmlEl::readfile: %ld -> %ld from '%s'\n", buf.size(), content.size(), full_fname.c_str());

	return true;
}

bool CSgmlEl::parse(std::string &path)
{
	PSgmlEl Child, Parent, Next = 0;
	size_t i, j, lins, line;
	size_t ls, le, rs, re, empty;

	std::wstring src;
	if (!readfile(path, src))
		return false;

	// start object - base
	type = EBASEEL;
	Next = this;

	lins = line = 0;
	for (i = 0; i < src.size(); i++)
	{
		// comments
		if (i+4 < src.size() && src[i] == '<' && src[i+1] == '!' && src[i+2] == '-' && src[i+3] == '-')
		{
			i += 4;
			while(i+3  < src.size() && (src[i] != '-' || src[i+1] != '-' || src[i+2] != '>'))
				i++;
			i+=3;
		}

		line = i;

		if (i >= src.size()-1 || src[i] == '<')
		{
			while (line > lins)
			{
				// linear
				j = lins;
				while (j < i && iswspace(src[j]))
					j++;
				if (j == i)
					break; // empty text
				Child = createnew(EPLAINEL,0,Next);
				Child->init();
				Child->setcontent(src.c_str() + lins, i - lins);
				Next = Child;
				break;
			}
			if (i + 1 >= src.size())
				continue;

			// start or single tag
			if (src[i+1] != '/')
			{
				Child = createnew(ESINGLEEL,NULL,Next);
				Child->init();
				Next  = Child;
				j = i+1;
				while (i < src.size() && src[i] != '>' && !iswspace(src[i]))
					i++;
				// Child->name = new wchar_t[i-j+1];
				if (i-j > MAXTAG)
					i = j + MAXTAG - 1;
				wcsncpy(Child->name, src.c_str()+j, i-j);
				Child->name[i-j] = 0;
				// parameters
				Child->parnum = 0;
				while (i < src.size() && src[i] != '>' && Child->parnum < MAXPARAMS)
				{
					ls = i;
					while (ls < src.size() && iswspace(src[ls]))
						ls++;
					le = ls;
					while (le < src.size() && !iswspace(src[le]) && src[le]!='>' && src[le]!='=')
						le++;
					rs = le;
					while (rs < src.size() && iswspace(src[rs]))
						rs++;
					empty = 1;
					if (src[rs] == '=')
					{
						empty = 0;
						rs++;
						while (rs < src.size() && iswspace(src[rs]))
							rs++;
						re = rs;
						if (src[re] == '"')
						{
							while (re < src.size() && src[++re] != '"')
								;
							rs++;
							i = re+1;
						}
						else if (src[re] == '\'')
						{
							while (re < src.size() && src[++re] != '\'')
								;
							rs++;
							i = re+1;
						} else
						{
							while (re < src.size() && !iswspace(src[re]) && src[re] != '>')
								re++;
							i = re;
						}
					} else
						i = re = rs;

					if (ls == le)
						continue;
					if (rs == re && empty)
					{
						rs = ls;
						re = le;
					}

					int pn = Child->parnum;
					Child->params[pn][0] = new wchar_t[le-ls+1];
					wcsncpy(Child->params[pn][0], src.c_str()+ls, le-ls);
					Child->params[pn][0][le-ls] = 0;

					Child->params[pn][1] = new wchar_t[re-rs+1];
					wcsncpy(Child->params[pn][1], src.c_str()+rs, re-rs);
					Child->params[pn][1][re-rs] = 0;

					Child->parnum++;

					substquote(Child->params[pn], L"&lt;", L'<');
					substquote(Child->params[pn], L"&gt;", L'>');
					substquote(Child->params[pn], L"&amp;", L'&');
					substquote(Child->params[pn], L"&quot;", L'"');
				}
				lins = i+1;

				// include
				if (wcscasecmp(Child->name, L"xi:include") == 0)
				{
					wchar_t *fn = Child->GetChrParam(L"href");
					if (fn)
					{
						std::string inc_path = path;
						size_t p = inc_path.rfind('/');
						if (p != std::string::npos)
							inc_path.resize(p + 1);
						inc_path+= Wide2MB(fn);

						std::wstring inc_content;
						if (readfile(inc_path, inc_content)) {
							src.insert(i + 1, inc_content);
						}
					}
				}
			} else
			{  // end tag
				j = i+2;
				i += 2;
				while (i < src.size() && src[i] != '>' && !iswspace(src[i]))
					i++;
				int cn = 0;
				for (Parent = Next; Parent->eprev; Parent = Parent->eprev, cn++)
				{
					if (!*Parent->name)
						continue;
					size_t len = wcslen(Parent->name);
					if (len != i-j)
						continue;
					if (Parent->type != ESINGLEEL || wcsncasecmp( (wchar_t*)src.c_str() + j, Parent->name, len))
						continue;
					break;
				}

				if (Parent && Parent->eprev)
				{
					Parent->echild = Parent->enext;
					Parent->chnum = cn;
					Parent->type = EBLOCKEDEL;
					Child = Parent->echild;
					if (Child)
						Child->eprev = 0;
					while(Child)
					{
						Child->eparent = Parent;
						Child = Child->enext;
					}
					Parent->enext = 0;
					Next = Parent;
				}
				while (i < src.size() && src[i] != '>')
					i++;
				lins = i+1;
			}
		}
	}

	return true;
}

void CSgmlEl::substquote(SParams par, const wchar_t *sstr, wchar_t c)
{
	int len = (int)wcslen(sstr);
	int plen = (int)wcslen(par[1]);

	for (int i = 0; i <= plen - len; i++)
	{
		if (!wcsncmp(par[1]+i, sstr, len))
		{
			par[1][i] = c;
			for(int j = i+1; j <= plen - len + 1; j++)
				par[1][j] = par[1][j+len-1];
			plen -= len - 1;
		}
	}
}

bool CSgmlEl::setcontent(const wchar_t *src, int sz)
{
	content = new wchar_t[sz+1];
	memmove(content, src, sz);
	content[sz] = 0;
	contentsz = sz;
	return true;
}

void CSgmlEl::insert(PSgmlEl El)
{
	El->eprev = this;
	El->enext = this->enext;
	El->eparent = this->eparent;
	if (this->enext)
		this->enext->eprev = El;
	this->enext = El;
}

// recursive deletion
void CSgmlEl::destroylevel()
{
	if (enext)
		enext->destroylevel();
	delete this;
}

PSgmlEl CSgmlEl::parent()
{
	return eparent;
}

PSgmlEl CSgmlEl::next()
{
	return enext;
}

PSgmlEl CSgmlEl::prev()
{
	return eprev;
}

PSgmlEl CSgmlEl::child()
{
	return echild;
}

ElType  CSgmlEl::gettype()
{
	return type;
}

wchar_t *CSgmlEl::getname()
{
	if (!*name)
		return NULL;
	return name;
}

wchar_t *CSgmlEl::getcontent()
{
	return content;
}

int CSgmlEl::getcontentsize()
{
	return contentsz;
}

wchar_t* CSgmlEl::GetParam(int no)
{
	if (no >= parnum)
		return NULL;
	return params[no][0];
}

wchar_t* CSgmlEl::GetChrParam(const wchar_t *par)
{
	for (int i = 0; i < parnum; i++)
	{
		if (!wcscasecmp(par,params[i][0]))
			return params[i][1];
    }
	return NULL;
}

bool CSgmlEl::GetIntParam(const wchar_t *par, int *result)
{
	double res = 0;
	for (int i=0; i < parnum; i++)
	{
		if (!wcscasecmp(par,params[i][0]))
		{
			bool b = get_number(params[i][1],&res);
			*result = b ? (int)res : 0;
			return b;
		}
	}
	return false;
}

bool CSgmlEl::GetFloatParam(const wchar_t *par, double *result)
{
	double res;
	for (int i = 0; i < parnum; i++)
	{
		if (!wcscasecmp(par,params[i][0]))
		{
			bool b = get_number(params[i][1],&res);
			*result = b ? (double)res : 0;
			return b;
		}
	}
	return false;
}

PSgmlEl CSgmlEl::search(const wchar_t *TagName)
{
	PSgmlEl Next = this->enext;
	while(Next)
	{
		if (!wcscasecmp(TagName,Next->name))
			return Next;
		Next = Next->enext;
	}
	return Next;
}

PSgmlEl CSgmlEl::enumchilds(int no)
{
	PSgmlEl El = this->echild;
	while(no && El)
	{
		El = El->enext;
		no--;
	}
	return El;
}

PSgmlEl CSgmlEl::fprev()
{
	PSgmlEl El = this;
	if (!El->eprev)
		return El->eparent;
	if (El->eprev->echild)
		return El->eprev->echild->flast();
	return El->eprev;
}

PSgmlEl CSgmlEl::fnext()
{
	PSgmlEl El = this;
	if (El->echild)
		return El->echild;
	while(!El->enext)
	{
		El = El->eparent;
		if (!El)
			return 0;
	}
	return El->enext;
}

PSgmlEl CSgmlEl::ffirst()
{
	PSgmlEl Prev = this;
	while(Prev)
	{
		if (!Prev->eprev)
			return Prev;
		Prev = Prev->eprev;
	}
	return Prev;
}

PSgmlEl CSgmlEl::flast()
{
	PSgmlEl Nxt = this;
	while(Nxt->enext || Nxt->echild)
	{
		if (Nxt->enext)
		{
			Nxt = Nxt->enext;
			continue;
		}
		if (Nxt->echild)
		{
			Nxt = Nxt->echild;
			continue;
		}
	}
	return Nxt;
}
