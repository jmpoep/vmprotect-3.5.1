#!/usr/bin/python

import pyblog, codecs, json, os, re, sys, time

SRC_PATH = "."
STRUCT_FILE = sys.argv[1]

##########################################################################################
# delete old pages

def find_user_manual_id(root_title, all_pages):
	for page in all_pages:
		if page['page_title'] == root_title:
			return int(page['page_id'])
	raise Exception('Can\'t find user manual page')

def find_children(all, root_id):
	children = []
	prev = [root_id]
	current = []
	while len(prev) > 0:
		for p in all:
			this_id = int(p['page_id'])
			parent_id = int(p['page_parent_id'])
			if prev.count(parent_id) > 0:
				current.append(this_id)
		children.extend(prev)
		prev = current
		current = []
	children.extend(prev)
	children.remove(root_id)
	return children

def delete_pages(blog, pages):
	for p in pages:
		try:
			blog.delete_page(p)
			#time.sleep(8)
		except pyblog.BlogError as text:
			print ("warning while attempting to delete the page %d: %s" % (p, text))
		except xml.parsers.expat.ExpatError as text:
			print ("xerror: %s" % (text))

##########################################################################################
# create new pages

def load_structure():
	f = codecs.open(STRUCT_FILE, 'r', 'utf-8')
	return json.load(f)

def get_good_image_link(link):
	parts = link.split('/')
	return "/usermanual/" + parts[len(parts) - 1]

def remove_newlines(text):
	lines = text.splitlines()
	nlines = []
	line = ""
	in_pre = False
	for l in lines:
		if in_pre:
			nlines.append(l)
			if re.search("</pre", l):
				in_pre = False
				nlines.append("")
			continue

		l = l.strip()
		if re.search("<pre", l):
			if line != "":
				nlines.append(line)
				line = ""
			nlines.append("")
			nlines.append(l)
			in_pre = True
			continue

		if l == "":
			if line != "":
				nlines.append(line)
			line = ""
			continue

		if line != "":
			line = line + " "
		line = line + l

	if line != "":
		nlines.append(line)
	
	return "\n".join(nlines)


def filter_content(text):
	# remove newlines
	text = remove_newlines(text)

	# adjust images
	images = set(re.findall('<img.*?src\s*=\s*"([^"]*)"', text, re.M + re.DOTALL))
	for l in images:
		text = text.replace(l, get_good_image_link(l))

	# remove footer
	text = re.sub('([\r\n\s]*<br[^>]*>[\r\n\s]*)+<hr[^>]*>[\s\n\r]*<div.*Copyright.+$', '', text)

	# that's all
	return text


def load_single_page(p):
	print ("\nprocessing page %s" % (p['file']))
	content = codecs.open(os.path.join(SRC_PATH, p['file']), 'r', 'utf-8').read()
	title = p['title']
	m_title = re.search('<title>(.*)</title>', content, re.M + re.DOTALL)
	if m_title: 
		title = m_title.group(1)
		title = title.replace('&quot;', '"')
		p['title'] = title

	m_content = re.search('<body>(.*)</body>', content, re.M + re.DOTALL)
	if m_content:
		content = m_content.group(1)
	else:
		content = "not found"
	content = filter_content(content)
	p['content'] = content


def load_pages_content(pages):
	for p in pages:
		load_single_page(p)
		if 'children' in p:
			load_pages_content(p['children'])

def print_pages_content(pages):
	for p in pages:
		print ("file %s" % (p['file']))
		print ("page %s, title = %s, content:\n%s\n\n" % (p['file'], p['title'], repr(p['content'])))
		if 'children' in p:
			print_pages_content(p['children'])


def create_single_page(blog, title, content, parent, order):
	print ("creating page: %s" % (title))
	try:
		query = {'wp_page_parent_id': parent, 'title': title, 'description': content, 'mt_allow_comments': 0, 'mt_allow_pings': 0, 'publish': 1, 'wp_page_order': order}
		return blog.new_page(query)
	except pyblog.BlogError as text:
		print ("error: %s" % (text))
	except xml.parsers.expat.ExpatError as text:
		print ("xerror: %s" % (text))

def create_new_pages(blog, root, pages):
	cnt = 0
	for p in pages:
		title = p['title']
		file = p['file']
		content = p['content']
		id = create_single_page(blog, title, content, root, cnt)
		#time.sleep(8)
		wp_page = blog.get_page(id)
		p['id'] = id
		p['link'] = wp_page['link']
		if 'children' in p:
			create_new_pages(blog, id, p['children'])
		cnt = cnt + 1

def update_single_page(blog, page):
	print ("updating page %s" % (page['title']))
	try:
		query = {'title': page['title'], 'description': page['content']}
		blog.edit_page(page['id'], query)
	except pyblog.BlogError as text:
		print ("error: %s" % (text))
	except xml.parsers.expat.ExpatError as text:
		print ("xerror: %s" % (text))

def update_pages(blog, pages):
	for p in pages:
		update_single_page(blog, p)
		time.sleep(10)
		if 'children' in p:
			update_pages(blog, p['children'])

def find_page_in_all_exact(fname, all_pages):
	for p in all_pages:
		if fname == p['file']: return p['link']
		if 'children' in p:
			res = find_page_in_all_exact(fname, p['children'])
			if res: return res
	return None

def find_page_in_all_by_name(fname, all_pages):
	for p in all_pages:
		if os.path.basename(fname) == os.path.basename(p['file']):
			return p['link']
		if 'children' in p:
			res = find_page_in_all_by_name(fname, p['children'])
			if res: return res
	return None

def find_target(fname, link, all_new_pages):
	dir = os.path.dirname(fname)
	tgt = os.path.join(dir, link)
	tgt = os.path.normpath(tgt)
	tgt = tgt.replace('\\', '/')
	good_tgt = find_page_in_all_exact(tgt, all_new_pages)
	if good_tgt:
		return good_tgt

	good_tgt = find_page_in_all_by_name(tgt, all_new_pages)
	if good_tgt:
		return good_tgt
	else:
		return "--- unknown link ---"

def process_links_on_page(page, all_new_pages):
	text = page['content']
	links = re.findall('<a[^>]*href\s*=\s*"([^"#]*)', text, re.DOTALL)
	for l in links:
		if l == "": continue
		nl = l.replace('\\', '/')
		if re.match("(http|mailto):", nl): continue
		nl = find_target(page['file'], nl, all_new_pages)
		text = text.replace(l, nl)
	page['content'] = text

def process_links(pages, all_new_pages):
	for p in pages:
		process_links_on_page(p, all_new_pages)
		if 'children' in p:
			process_links(p['children'], all_new_pages)

def get_list_of_pages(pages):
	list = "\n<ul class=toc>\n"
	for p in pages:
		list = list + '<li><a href="' + p['link'] + '">' + p['title'] + '</a></li>\n'
		if 'children' in p:
			list = list + get_list_of_pages(p['children'])
	return list + "</ul>\n"

def update_main_page(blog, id, pages):
	print ('updating the main user manual page')

	list = '''
		<h1>User Manual</h1>

		The table of contents:
		
		<style> ul.toc li { margin-left: 2em; } #content>ul { padding-left: 0; } </style>
		
		'''
	list = list + get_list_of_pages(pages)

	try:
		query = {'description': list}
		blog.edit_page(id, query)
	except pyblog.BlogError as text:
		print ("error: %s" % (text))



##########################################################################################
# main code

print ("------------------")
test = '''
one
<pre>
two
	three
		four</pre>
	more
	some text
	<pre>
	some text
	one more
	</pre>

	a
	b
	c
'''

#print test
#test = remove_newlines(test)
#print "------------------"
#print test

#quit()

struct = load_structure()
new_pages = struct['children']
load_pages_content(new_pages)

#print_pages_content(new_pages)
#quit()

# blog = pyblog.WordPress('http://test.vmpsoft.com/xmlrpc.php', 'admin', '12345')
blog = pyblog.WordPress('http://vmpsoft.com/xmlrpc.php', 'uploader', 'lcRn4F29Rr4S')
all_pages = blog.get_page_list()
user_manual_id = find_user_manual_id(struct['root_title'], all_pages)
print ("user manual page has id %d" % (user_manual_id))
user_manual_children = find_children(all_pages, user_manual_id)
print ("user manual children pages are %s\ndeleting..." % (user_manual_children))
delete_pages(blog, user_manual_children) # to trash
delete_pages(blog, user_manual_children) # from trash
print ("done, ready to create the new structure")

create_new_pages(blog, user_manual_id, new_pages)

process_links(new_pages, new_pages)

update_pages(blog, new_pages)

update_main_page(blog, user_manual_id, new_pages)
