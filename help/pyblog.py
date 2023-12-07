#!/usr/bin/python


""""
  Copyright 2008 Ritesh Nadhani. All Rights Reserved.

  For license information please check the LICENSE page of the package.

"""

''' A library that provides python interface to '''

from xmlrpc import client
import urllib.request

# Helper function to check if URL exists

def checkURL(url):
    try: urllib.request.urlopen(url)
    except IOError: return 0
    return 1

class BlogError(Exception):
    
    '''Base class for Blog errors'''
    METHOD_NOT_SUPPORTED           = 'Method not (yet) supported'
    
    def __init__(self, msg):
        self.msg  = msg
        
    def __repr__(self):
        return self.msg   

    __str__ = __repr__   
        
class Blog(object):
    """
    Base class for all blog object. Python interface to various blogging API

    This and extending class are just simple encapsulators over XML-RPC. It does nothing but call the corresponding XMLRPC functions and check for error.
    
    Rightnow, it returns Python based dictionary as it is returned by the server but later on we maybe encapsulate the data using simplified custon object.
    """
    def __init__(self, serverapi, username, password, appkey):
        """
        Args:
            serverapi = URL to the XML-RPC API.
            username  = Username for the Blog account.
            password  = Password for the Blog account.
        """
        self.username   = username
        self.password   = password
        self.appkey = appkey    
        self.methods            = []

        # Check if URL exists
        if not checkURL(serverapi):
            raise BlogError('XML-RPC API URL not found.')

        # Connect to the api. Call listMethods to keep a dictionary of available methods
        self.server             = client.ServerProxy(serverapi)
        self.list_methods()

    def list_methods(self):
        """Call systen.listMethods on server.

        Returns:
            List of XML-RPC methods implemented by the server. 
        """
        if not len(self.methods):
            try:
                self.methods = self.server.system.listMethods()
            except client.Fault as fault:
                raise BlogError(fault.faultString)

        return self.methods.sort()

    def execute(self, methodname, *args):

        """
        Callback function to call the XML-RPC method

        Args:
           methodname = XML-RPC methodname.
           args = Arguments to the call. 
        """
        if not methodname in self.methods:
            raise BlogError(BlogError.METHOD_NOT_SUPPORTED)

        try:
            r = getattr(self.server, methodname)(args)
        except client.Fault as fault:
            raise BlogError(fault.faultString)

        return r
        
    def is_method_available(self, methodname):
        """Returns if a method is supported by the XML-RPC server"""
        if methodname in self.methods:
            return True
        else:
            return False

class Blogger(Blog):
    """
    A Python interface to the blogger API.
    """

    def __init__(self, serverapi, username, password):
        raise BlogError("This class has not yet been implemented")

class MetaWeblog(Blog):
    """
    Python interface to Metaweblog API
    This class extends Blog to implement metaWeblog API
    """

    def __init__(self, serverapi, username, password, appkey='0x001'):
        Blog.__init__(self, serverapi, username, password, appkey)
        
    def get_recent_posts(self, numposts=10, blogid=1):
        """
        Returns 'numposts' number of recent post for the blog identified by 'blogid'
        
        Args:
            numposts = Number of posts to be returned [optional]
            blogid   = id of thr blog
            """
        return self.execute('metaWeblog.getRecentPosts', blogid, self.username, self.password, numposts)
    
    def get_post(self, postid):
        """
        Returns dictionary based post content corresponding to postid.
        
        Args:
            postid = Unique identifier for the post
        """
        return self.execute('metaWeblog.getPost', postid, self.username, self.password)
        
    def new_post(self, content, publish=True, blogid=1):
        """
        New post

        Args:
            content = Dictionary containing post dat.
            Publish = Publish status.
            blogid  = Blog ID
        
        """
        return self.execute('metaWeblog.newPost', blogid, self.username, self.password, content, publish)

    def edit_post(self, postid, newpost, publish=True):
        """
        Edits a post identified by postid with content passed in newpost

        Args:
            postid = post identified by postid.
            newpost = dictionary with content details about the new post.
            Publish = Publish status.        
        """
        return self.execute('metaWeblog.editPost', postid, self.username, self.password, newpost, publish)

    def delete_post(self, postid, publish=True):
        """
        Deletes a post identified by postid

        Args:
            postid = post identified by postid.
            Publish = Publish status.

        """
        return self.execute('metaWeblog.deletePost', self.appkey, postid, self.username, self.password, publish)

    def get_categories(self, blogid=1):
        """
        Returns a list of categories.

        Args:
            blogid = Blog ID
        """
        return self.execute('metaWeblog.getCategories', blogid, self.username, self.password)

    def get_users_blogs(self):
        """
        Returns a list of blogs associated by the user.

        """
        return self.execute('metaWeblog.getUsersBlogs', self.appkey, self.username, self.password)

    def new_media_object(self, new_object, blogid=1):
        """
        Args:
            new_object = Structure containing information about new media object to be uploaded
            blogid = Blog ID
            
        Returns:
            URL to the uploaded file

        """
        return self.execute('metaWeblog.newMediaObject', blogid, self.username, self.password, new_object)
        
    def get_template(self, templateType, blogid=1):
        """Returns the template type identifed by templateType"""
        return self.execute("metaWeblog.getTemplate", self.appkey, blogid, self.username, self.password, templateType)
        
    def set_template(self, template, templateType, blogid=1):
        
        """Sets the new template value for templateType"""
        return self.execute("metaWeblog.setTemplate", self.appkey, blogid, self.username, self.password, template, templateType)        
        
class WordPress(MetaWeblog):
    """
    Python interface to Wordpress API
    Wordpress basically implements all MetaWebLog and extends it by providing it with its methods.
    """

    def __init__(self, serverapi, username, password):
        MetaWeblog.__init__(self, serverapi, username, password)
        
    def get_post_status_list(self, blogid=1):
        """
        ( Draft, Pending Review, Private, Published ).Returns a dict of all the valid post statuses ( draft, pending, private, publish ) and their descriptions 
        ( Draft, Pending Review, Private, Published ).
        """
        return self.execute('wp.getPostStatusList', blogid, self.username, self.password)

    def get_authors(self, blogid=1):
        """
            Get a list of users for the blog.
        """
        return self.execute('wp.getAuthors', blogid, self.username, self.password)
        
    def new_page(self, content, publish=True, blogid=1):
        """
        Args:
            content - Dictionary of new content
        """
        return self.execute('wp.newPage', blogid, self.username, self.password, content, publish)
        
    def edit_page(self, pageid, content, publish=True, blogid=1):
        """
        Args:
            pageid = Page to edit
            content - Dictionary of new content
        """
        return self.execute('wp.editPage', blogid, pageid, self.username, self.password, content, publish)

    def delete_page(self, pageid, blogid=1):
        """
        Args:
            pageid = Page to delete
        """
        return self.execute('wp.deletePage', blogid, self.username, self.password, pageid)

    def get_pages(self, blogid=1):
        """
        Returns a list of the most recent pages in the system.
        """
        return self.execute('wp.getPages', blogid, self.username, self.password)

    def get_page(self, pageid, blogid=1):
        """
        Returns the content of page identified by pageid
        """
        return self.execute('wp.getPage', blogid, pageid, self.username, self.password)
        
    def get_page_list(self, blogid=1):
        """
        Get an list of all the pages on a blog. Just the minimum details, lighter than wp.getPages.
        """
        return self.execute('wp.getPageList', blogid, self.username, self.password)
        
    def get_page_status_list(self, blogid=1):
        """Returns a dict of all the valid page statuses ( draft, private, publish ) and their descriptions ( Draft, Private, Published)"""
        
        return self.execute('wp.getPageStatusList', blogid, self.username, self.password)        
        
    def new_category(self, content, blogid=1):
        """
        Args:
            content = Dictionary content having data for new category.
            
        Returns id of new value
        """
        return self.execute('wp.newCategory', blogid, self.username, self.password, content)

    def delete_category(self, catid, blogid=1):
        """
        Args:
            catid = Category ID
            blogid = Blog ID

        """
        return self.execute('wp.deleteCategory', blogid, self.username, self.password, catid)
        
    def get_comment_count(self, postid=0, blogid=1):
        """
        Provides a struct of all the comment counts ( approved, awaiting_moderation, spam, total_comments ) for a given postid.
        The postid parameter is optional (or can be set to zero), if it is not provided then the same struct is returned, but for the 
        entire blog instead of just one post
        """
         
        return self.execute('wp.getCommentCount', blogid, self.username, self.password, postid)
        
    def get_users_blogs(self):
        """
        Returns a list of blogs associated by the user.
        """
        return self.execute('wp.getUsersBlogs', self.username, self.password)

    def get_options(self, options=[], blogid=1):
        """
        Return option details.
        
        The parameter options, list, is optional. If it is not included then it will return all of the option info that we have. 
        With a populated list, each field is an option name and only those options asked for will be returned.
        """
        return self.execute('wp.getOptions', blogid, self.username, self.password, options)

    def set_options(self, option, blogid=1):
        """
        That option parameter is option name/value pairs. The return value is same as if you called wp.getOptions asking for the those option names, 
        only they'll include the new value. If you try to set a new value for an option that is read-only, it will silently fail and you'll get the original
        value back instead of the new value you attempted to set.
        """
        return self.execute('wp.setOptions', blogid, self.username, self.password, option)
        
    def suggest_categories(self, category, max_results=10, blogid=1):
        """Returns a list of dictionaries of categories that start with a given string."""
        return self.execute('wp.suggestCategories', blogid, self.username, self.password, category, max_results)
        
    def upload_file(self, data, blogid=1):
        """
        Upload a file.
        
        Data contains values as documented at http://codex.wordpress.org/XML-RPC_wp#wp.getCategories
        """
        return self.execute('wp.uploadFile', blogid, self.username, self.password, data)

class MovableType(MetaWeblog):
    """
    A Python interface to the MovableType API.
    """

    def __init__(self, serverapi, username, password):
        raise BlogError("This class has not yet been implemented")
        
def main():
    pass
    
if __name__ == '__main__':
    main()
