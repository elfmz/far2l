from requests import get
from time import sleep
from urllib.parse import quote
from json import loads

all_params = {"dt": ["t", "bd", "ex", "ld", "md", "qca", "rw", "rm", "ss", "t", "at"], "dj": "1", "source": "bubble"}
general_params = {"dt": ["t", "bd", "qca","t", "at",'ex','md','rw'],"dj": "1"}
# dt parameter:
#   t  - translation of source text
#   at - alternate translations
#   rm - transcription / transliteration of source and translated texts
#   bd - dictionary, in case source text is one word (you get translations with articles, reverse translations, etc.)
#   md - definitions of source text, if it's one word
#   ss - synonyms of source text, if it's one word
#   ex - examples
#   rw - See also list.
# dj - Json response with names. (dj=1)

class GoogleTranslator:
    def __init__(self,name,base_url,general_params):
        """Google Translator can support multiple source."""
        self.name=name
        self.url = base_url
        self.base_parames=general_params
        self.client_list={
            'nottk':['gtx','at'],
            'ttk':['webapp'],
            'clients5':['dict-chrome-ex']
        }
        self.client_list_state=self.client_list.copy()

    @staticmethod
    def convert_qtext(text):
        return quote(text)

    @staticmethod
    def calculate_tkid(text):
        return tk.calculate_token(text)

    def __call__(self,input_text, sourcelanguage='auto',targetlanguage='tr',ttk_enable=False):
        HEADERS = {
            "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36",
            "Accept": "*/*",
            "Accept-Language": "en-US,en-GB; q=0.5",
            "Accept-Encoding": "gzip, deflate",
            "Content-Type": "application/x-www-form-urlencoded; application/json; charset=UTF-8",
            "Connection": "keep-alive"
        }
        result_dict={}
        #text = self.convert_qtext(input_text)
        self.parames=general_params
        self.parames['sl']=sourcelanguage
        self.parames['tl']=targetlanguage
        self.parames['hl']=targetlanguage
        self.parames['q']=input_text
        self.parames['ie']='UTF-8'
        self.parames['oe']='UTF-8'
        mode=''
        if self.name == 'clients5' and not ttk_enable:
            mode='clients5'
        elif ttk_enable:
            mode = 'ttk'
            tkid = self.calculate_tkid(input_text)
            self.parames['tk']=tkid
        else:
            mode='nottk'
        #print(mode,self.client_list[mode])
        self.parames['client']=self.client_list[mode][0]

        try:
            req = get(self.url,params=self.parames,headers=HEADERS)
            status_code = req.status_code
        except Exception :
            return 400,None

        if status_code == 400:
            return status_code,None

        while  status_code != 200:
            self.client_list[mode].pop(0)
            if self.client_list[mode] == []:
                return False,None
            self.parames['client']=self.client_list[mode][0]
            req = get(self.url,params=self.parames,headers=HEADERS)
            status_code = req.status_code

        return status_code,self.parser(req.text)


    def parser(self,request_text):
        req = loads(request_text)
        if 'definitions' in req.keys():
            definitions={}
            for p in  req['definitions']:
                definitions[p['pos']]= [ {'detail':g['gloss'],'example':g['example'] if 'example' in g.keys() else None} for g in p['entry']]
        else:
            definitions =None
        if 'examples' in req.keys():
            examples=[e['text'].replace('<b>','').replace('</b>','') for e in req['examples']['example']]
        else:
            examples=None
        detect_language = req['ld_result']['extended_srclangs'][0]#req['src']

        if self.name  == 'clients5':
            req['sentences']=req['sentences'][:-1]
        result = [ trans['trans'] for trans in req['sentences']]

        if 'spell' in req and 'spell_res' in req['spell']:
            revise=req['spell']['spell_res']
        else:
            revise=None

        if 'dict' in req:
            dict_list = req['dict']
            allresult = [{'pos': dic['pos'],'terms':dic['terms'] } for dic in dict_list]
        else:
            allresult=[]
        result_dict = {}
        result_dict['result']=result
        result_dict['all_result']=allresult
        result_dict['detect_language']=detect_language
        result_dict['revise']=revise
        result_dict['definition']=definitions
        result_dict['example']=examples
        return result_dict

#service
service_dict={
    'com_apis':'https://translate.googleapis.com/translate_a/single',
    'clients5':'https://clients5.google.com/translate_a/t',
    'com':'https://translate.google.com/translate_a/single',
    'tw':'https://translate.google.com.tw/translate_a/single',
    'cn':'https://translate.google.cn/translate_a/single',
}

translator_list = [GoogleTranslator(name,url,general_params) for name,url in service_dict.items()]

translator_idx=0
ttk_enable=False

RETRY_TIME=60*5

def get_translate(inputtext, sourcelanguage='auto',targetlanguage='zh-TW'):
    global ttk_enable,translator_idx,RETRY_TIME

    status,result_dict=translator_list[translator_idx](inputtext, sourcelanguage,targetlanguage,ttk_enable)

    if status == 200:
        return result_dict
    elif status == 400:
        return None
    if True:
        return None
    else:
        translator_idx +=1
        if translator_idx < len(translator_list):
            return get_translate(inputtext, sourcelanguage,targetlanguage)
        elif not ttk_enable:
            translator_idx=0
            ttk_enable=True
            return get_translate(inputtext, sourcelanguage,targetlanguage)
        else:
            sleep(RETRY_TIME)
            translator_idx=0
            ttk_enable=False
            return get_translate(inputtext, sourcelanguage,targetlanguage)

if __name__ == '__main__':
    print(get_translate("And do beautifull things", "en", "pl-PL"))
