#include <chrono>
#include "listener.hpp"
#include <intrusive_ptr.hpp>


namespace stream_cloud {
    namespace providers {
        namespace http_server {

            using clock = std::chrono::steady_clock;

            constexpr const char *dispatcher = "dispatcher";

            listener::listener(boost::asio::io_context &ioc, tcp::endpoint endpoint, actor::actor_address pipe_,
                               std::shared_ptr<std::string const> const &doc_root) :
                    acceptor_(ioc),
                    socket_(ioc),
                    pipe_(pipe_),
                    doc_root_(doc_root) {
                boost::system::error_code ec;

                // Open the acceptor
                acceptor_.open(endpoint.protocol(), ec);
                if (ec) {
                    //  fail(ec, "open");
                    return;
                }

                // Allow address reuse
                acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
                if (ec) {
                    //  fail(ec, "set_option");
                    return;
                }

                // Bind to the server address
                acceptor_.bind(endpoint, ec);
                if (ec) {
                    //  fail(ec, "bind");
                    return;
                }

                // Start listening for connections
                acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
                if (ec) {
                    // fail(ec, "listen");
                    return;
                }
            }

            void listener::run() {
                if (!acceptor_.is_open()) {
                    return;
                }

                do_accept();
            }

            void listener::do_accept() {
                acceptor_.async_accept(
                        socket_,
                        std::bind(
                                &listener::on_accept,
                                shared_from_this(),
                                std::placeholders::_1));
            }

            void listener::on_accept(boost::system::error_code ec) {
                if (ec) {
                    // fail(ec, "accept");
                } else {
                    std::make_shared<http_session>(std::move(socket_), *this)->run();
                }

                // Accept another connection
                do_accept();
            }

            void listener::add_trusted_url(std::string name) {
                trusted_url.emplace(std::move(name));
            }

            void listener::remove_trusted_url(std::string name) {
                trusted_url.erase(name);
            }

            auto listener::check_url(const std::string &url) const -> bool {
                ///TODO: not fast
                auto start = url.begin();
                // trusted_url.find(url) != trusted_url.end();
                ++start;
                return (trusted_url.find(std::string(start, url.end())) != trusted_url.end());
            }

            // Append an HTTP rel-path to a local filesystem path.
            std::string listener::path_cat(boost::beast::string_view base, boost::beast::string_view path) const {
                if (base.empty())
                    return path.to_string();
                std::string result = base.to_string();

                char constexpr path_separator = '/';
                if (result.back() == path_separator)
                    result.resize(result.size() - 1);
                result.append(path.data(), path.size());

                return result;
            }

            boost::beast::string_view listener::mime_type(boost::beast::string_view path) const {
                using boost::beast::iequals;
                auto const ext = [&path] {
                    auto const pos = path.rfind(".");
                    if (pos == boost::beast::string_view::npos)
                        return boost::beast::string_view{};
                    return path.substr(pos);
                }();
                if (iequals(ext, ".htm")) return "text/html";
                if (iequals(ext, ".html")) return "text/html";
                if (iequals(ext, ".php")) return "text/html";
                if (iequals(ext, ".css")) return "text/css";
                if (iequals(ext, ".txt")) return "text/plain";
                if (iequals(ext, ".js")) return "application/javascript";
                if (iequals(ext, ".json")) return "application/json";
                if (iequals(ext, ".xml")) return "application/xml";
                if (iequals(ext, ".swf")) return "application/x-shockwave-flash";
                if (iequals(ext, ".flv")) return "video/x-flv";
                if (iequals(ext, ".png")) return "image/png";
                if (iequals(ext, ".jpe")) return "image/jpeg";
                if (iequals(ext, ".jpeg")) return "image/jpeg";
                if (iequals(ext, ".jpg")) return "image/jpeg";
                if (iequals(ext, ".gif")) return "image/gif";
                if (iequals(ext, ".bmp")) return "image/bmp";
                if (iequals(ext, ".ico")) return "image/vnd.microsoft.icon";
                if (iequals(ext, ".tiff")) return "image/tiff";
                if (iequals(ext, ".tif")) return "image/tiff";
                if (iequals(ext, ".svg")) return "image/svg+xml";
                if (iequals(ext, ".svgz")) return "image/svg+xml";
                return "application/text";
            }


            auto listener::operator()(http::request<http::string_body> &&req,
                                      const std::shared_ptr<http_session> &session) -> void {


                // Returns a bad request response
                auto const bad_request =
                        [&req](boost::beast::string_view why) {
                            http::response<http::string_body> res{http::status::bad_request, req.version()};
                            res.set(http::field::content_type, "text/html");
                            res.keep_alive(req.keep_alive());
                            res.body() = why.to_string();
                            res.prepare_payload();
                            return res;
                        };

                // Returns a not found response
                auto const not_found =
                        [&req](boost::beast::string_view target) {
                            http::response<http::string_body> res{http::status::not_found, req.version()};
                            res.set(http::field::content_type, "text/html");
                            res.keep_alive(req.keep_alive());
                            res.body() = "The resource '" + target.to_string() + "' was not found.";
                            res.prepare_payload();
                            return res;
                        };

                // Returns a server error response
                auto const server_error =
                        [&req](boost::beast::string_view what) {
                            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
                            res.set(http::field::content_type, "text/html");
                            res.keep_alive(req.keep_alive());
                            res.body() = "An error occurred: '" + what.to_string() + "'";
                            res.prepare_payload();
                            return res;
                        };

                // Make sure we can handle the method
                if (req.method() != http::verb::get &&
                    req.method() != http::verb::head)
                    return session->send(bad_request("Unknown HTTP-method"));


                // Request path must be absolute and not contain "..".
                if (req.target().empty() ||
                    req.target()[0] != '/' ||
                    req.target().find("..") != boost::beast::string_view::npos)
                    return session->send(bad_request("Illegal request-target"));


                // Поддомен
                std::string sub_domain;
                std::string root_path;
                auto const host = req.at(http::field::host);
                auto const pos_domain = host.find_first_of(".");

                if (pos_domain == boost::beast::string_view::npos) {
                    sub_domain = {};
                    root_path = *doc_root_;
                } else {
                    sub_domain =  host.substr(0, pos_domain).to_string();
                    root_path =  *doc_root_ + "/" + sub_domain;
                }

                std::string path;

                if (req.target().rfind(".") == boost::beast::string_view::npos) {
                    path = path_cat(root_path, "/index.html");
                } else {
                    path  = path_cat(root_path, req.target());
                }



                // Attempt to open the file
                boost::beast::error_code ec;
                http::file_body::value_type body;
                body.open(path.c_str(), boost::beast::file_mode::scan, ec);

                // Handle the case where the file doesn't exist
                if (ec == boost::beast::errc::no_such_file_or_directory)
                    return session->send(not_found(req.target()));

                // Handle an unknown error
                if (ec)
                    return session->send(server_error(ec.message()));

                // Cache the size since we need it after the move
                auto const size = body.size();

                // Respond to HEAD request
                if (req.method() == http::verb::head) {
                    http::response<http::empty_body> res{http::status::ok, req.version()};
                    res.set(http::field::content_type, mime_type(path));
                    res.content_length(size);
                    res.keep_alive(req.keep_alive());
                    return session->send(std::move(res));
                }

                // Respond to GET request
                http::response<http::file_body> res{
                        std::piecewise_construct,
                        std::make_tuple(std::move(body)),
                        std::make_tuple(http::status::ok, req.version())};
                res.set(http::field::content_type, mime_type(path));
                res.content_length(size);
                res.keep_alive(req.keep_alive());
                return session->send(std::move(res));

            }
        }
    }
}