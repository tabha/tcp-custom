

Demo
----

Features
---------
- tcp connection procedure
- loss simulation
- loss recovery management
- packet buffering over IP
- Adaptation of the RRT
- STOP AND WAIT


Rename `config.ini.example` inside the `config` folder to `config.ini` (or you can create a new `config/config.ini` file) then change the site settings there.

Create `YourUsername.ini` inside the `config/users` folder or simply rename the `username.ini.example` file and write down your password there:

````cfg
password = YourPassword
````

In addition, HTMLy support admin user role. To do so, simply add the following line to your choosen user:

````cfg
role = admin
````

Users assigned with the admin role can edit/delete all users' posts.

To access the admin panel, add `/login` to the end of your site's URL.
e.g. `www.yoursite.com/login`

### Lighttpd
The following is an example configuration for lighttpd:

````php
$HTTP["url"] =~ "^/config" {
  url.access-deny = ( "" )
}
$HTTP["url"] =~ "^/system/includes" {
  url.access-deny = ( "" )
}
$HTTP["url"] =~ "^/system/admin/views" {
  url.access-deny = ( "" )
}

url.rewrite-once = (
  "^/(themes|system|vendor)/(.*)" => "$0",
  "^/(.*\.php)" => "$0",

  # Everything else is handles by htmly
  "^/(.*)$" => "/index.php/$1"
)
````

### Nginx
The following is a basic configuration for Nginx:

````nginx
server {
  listen 80;

  server_name example.com www.example.com;
  root /usr/share/nginx/html;

  access_log /var/log/nginx/access.log;
  error_log /var/log/nginx/error.log error;

  index index.php;

  location ~ /config/ {
     deny all;
  }

  location / {
    try_files $uri $uri/ /index.php?$args;
  }

  location ~ \.php$ {
        fastcgi_pass   127.0.0.1:9000;
        fastcgi_index  index.php;
        fastcgi_param  SCRIPT_FILENAME   $document_root$fastcgi_script_name;
        include        fastcgi_params;
  }
}
````

Making a secure password
----------------------
Passwords can be stored in `username.ini` (where "username" is the user's username) in either plaintext, encryption algorithms supported by php `hash` or bcrypt (recommended). To generate a bcrypt encrypted password:
````
$ php -a
> echo password_hash('desiredpassword', PASSWORD_BCRYPT);
````
This will produce a hash which is to be placed in the `password` field in `username.ini`. Ensure that the `encryption` field is set to `password_hash`.


Both Online or Offline
----------------------
The built-in editor found in the admin panel, also provides you the ability to write to Markdown files offline by uploading them (see naming convention below) into the `content/username/blog/category/type/`:

* `username` must match `config/users/username.ini`.
* `category` must match the `category.md` inside `content/data/category/category.md` except the `uncategorized` category.
* `type` is the content type. Available content type `post`, `video`, `audio`, `link`, `quote`.

For static pages you can upload it to the `content/static` folder.

Category
--------
The default category is `Uncategorized` with slug `uncategorized` and you do not need to creating it inside `content/data/category/` folder. But if you write it offline and want to assign new category to specific post you need to creating it first before you can use those category, example `content/data/category/new-category.md` with the following content:

```html
<!--t New category title t-->
<!--d New category meta description d-->

New category info etc.
````
The slug for the new category is `new-category` (htmly removing the file extension). And for full file directory:
````
content/username/new-category/post/file.md
````

File Naming Convention
----------------------
When you write a blog post and save it via the admin panel, HTMLy automatically create a .md file extension with the following name, example:

````
2014-01-31-12-56-40_tag1,tag2,tag3_databaseless-blogging-platform-flat-file-blog.md
````

Here's the explanation (separated by an underscore):

- `2014-01-31-12-56-40` is the published date. The date format is `yyyy-mm-dd-hh-mm-ss`
- `tag1,tag2,tag3` are the tags, separated by commas
- `databaseless-blogging-platform-flat-file-blog` is the URL

For static pages, use the following format:

````
content/static/about.md
````

In the example above, the `/about.md` creates the URL: `www.yourblog.com/about`

Thus, if you write/create files offline, you must name the .md file in the format above.

For static subpages, use the following format:

````
content/static/about/me.md
````

This will create the URL: `www.yourblog.com/about/me`

Content Tags
-------------
If you are writing offline, you need specify the content tags below:

**Title**
```html
<!--t Title t-->
````

**Meta description**
```html
<!--d The meta description d-->
````

**Tags**

This is just the tags display and for the slug is in the filename.
```html
<!--tag Tag1,Tag2 tag-->
````

**Featured image**

Post with featured image.
```html
<!--image http://www.example.com/image-url/image.jpg image-->
````

**Featured youtube video**

Post with featured youtube video.
```html
<!--video https://www.youtube.com/watch?v=xxxxxxx video-->
````

**Featured soundcloud audio**

Post with featured soundcloud audio.
```html
<!--audio https://soundcloud.com/xxxx/audio-url audio-->
````

**Featured link**

Post with featured link.
```html
<!--link https://github.com/danpros/htmly link-->
````

**Featured quote**

Post with featured quote.
```html
<!--quote Premature Optimization is The Root of All Evil quote-->
````

**Example**

Example of how your post would look like:
```html
<!--t Here is the post title t-->
<!--d The meta description d-->
<!--tag Tag1,Tag2 tag-->
<!--video https://www.youtube.com/watch?v=xxxxxxx video-->

Paragraph 1

Paragraph 2 etc.
```

Credit
------
* [Martin Angelov](http://tutorialzine.com)
* [Xiaoying Riley](http://themes.3rdwavemedia.com)

Contribute
----------
1. Fork and edit
2. Submit pull request for consideration

Contributors
----------
- [HTMLy Contributors](https://github.com/danpros/htmly/graphs/contributors)

Copyright / License
-------------------
For copyright notice please read [COPYRIGHT.txt](https://github.com/danpros/htmly/blob/master/COPYRIGHT.txt). HTMLy is licensed under the GNU General Public License Version 2.0 (or later).


**Versions**
--------------


**Version 1**
-------------

-the first version implements a no-loss recovery version. Thus any packet lost during the transmission phase will not be retransmitted.

**Version 2**
---------------
-In version two, we are interested in an implementation that guarantees total reliability via a "Stop and Wait" type loss recovery mechanism.


**version 3**
------------
-a guarantee of "static" partial reliability via a "Stop and Wait" type loss recovery mechanism with "pre-wired" partial reliability, i.e. with a statically defined % of admissible losses


**Version 4**
--------------
-a connection establishment phase
-a guarantee of partial reliability through a "Stop and Wait" loss recovery mechanism, the % of allowable losses of which is now negotiated during the connection establishment phase


**Final Version**
-----------------
-Connection phase (since v4).
-Negotiation of the percentage of loss allowed (this negotiation is added in the header during the connection phase).
-Calculation of the RTT and adaptation of the value during communication (the maximum value is set here to 1ms as we are in local host).
   (at home with the ping localhost I have a rtt close to 0.43ms)
   in real communication will have to change the max. value and adapt to the network specification.
