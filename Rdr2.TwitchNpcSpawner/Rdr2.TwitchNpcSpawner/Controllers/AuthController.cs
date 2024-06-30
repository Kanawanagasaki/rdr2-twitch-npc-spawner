namespace Rdr2.TwitchNpcSpawner.Controllers;

using Rdr2.TwitchNpcSpawner.Services;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Microsoft.IdentityModel.Tokens;
using System.IdentityModel.Tokens.Jwt;
using System.Security.Claims;
using System.Text;
using System.Text.Json;
using System.Web;

[ApiController, Route("[controller]")]
public class AuthController(ILogger<AuthController> _logger, IConfiguration _conf, MySqlContext _db, JwtService _jwtService) : Controller
{
    private record RefreshResult(string access_token, string refresh_token, string[] scope, string token_type);
    [HttpGet("[action]")]
    public async Task<IActionResult> Validate()
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        using var http = new HttpClient();
        using var request = new HttpRequestMessage(HttpMethod.Get, "https://id.twitch.tv/oauth2/validate");
        request.Headers.Add("Authorization", "Bearer " + user.AccessToken);
        using var response = await http.SendAsync(request);

        if (response.StatusCode != System.Net.HttpStatusCode.OK)
        {
            var form = new FormUrlEncodedContent(new Dictionary<string, string?>
                {
                    { "client_id", _conf["Twitch:ClientId"] },
                    { "client_secret", _conf["Twitch:Secret"] },
                    { "grant_type", "refresh_token" },
                    { "refresh_token", user.RefreshToken }
                });
            using var request2 = new HttpRequestMessage(HttpMethod.Post, "https://id.twitch.tv/oauth2/token");
            request2.Content = form;
            using var response2 = await http.SendAsync(request2);
            if (response2.StatusCode != System.Net.HttpStatusCode.OK)
                return StatusCode(StatusCodes.Status401Unauthorized);

            var content = await response2.Content.ReadAsStringAsync();
            var refreshRes = JsonSerializer.Deserialize<RefreshResult>(content);
            if (refreshRes is null)
                return StatusCode(StatusCodes.Status500InternalServerError);

            user.AccessToken = refreshRes.access_token;
            user.RefreshToken = refreshRes.refresh_token;
            await _db.SaveChangesAsync();
        }

        return NoContent();
    }

    [HttpGet("[action]")]
    public IActionResult OAuthEndpoint()
    {
        string endpoint = "https://id.twitch.tv/oauth2/authorize"
            + "?response_type=code"
            + "&client_id=" + _conf["Twitch:ClientId"]
            + "&redirect_uri=" + HttpUtility.UrlEncode(_conf["Twitch:RedirectUrl"])
            + "&scope=" + HttpUtility.UrlEncode(_conf["Twitch:Scopes"]);
        return Content(endpoint, "text/plain", Encoding.UTF8);
    }

    private record OAuth2TokenResponse(string access_token, int expires_in, string id_token, string refresh_token, string[] scope, string token_type);
    private record IdToken(string aud, string azp, long exp, long iat, string iss, string sub, string? nonce, string? preferred_username);
    [HttpGet("[action]")]
    public async Task<IActionResult> Code()
    {
        if (Request.Query.ContainsKey("error"))
            return StatusCode(StatusCodes.Status401Unauthorized, new
            {
                error = Request.Query["error"].ToString(),
                errorMessage = Request.Query.ContainsKey("error_description") ? Request.Query["error_description"].ToString() : ""
            });
        else if (Request.Query.ContainsKey("code"))
        {
            var code = Request.Query["code"].ToString();
            using var form = new FormUrlEncodedContent(new Dictionary<string, string>
            {
                ["client_id"] = _conf["Twitch:ClientId"] ?? string.Empty,
                ["client_secret"] = _conf["Twitch:Secret"] ?? string.Empty,
                ["code"] = code,
                ["grant_type"] = "authorization_code",
                ["redirect_uri"] = _conf["Twitch:RedirectUrl"] ?? string.Empty
            });
            using var http = new HttpClient();
            using var response = await http.PostAsync("https://id.twitch.tv/oauth2/token", form);
            if (!response.IsSuccessStatusCode)
                return StatusCode((int)response.StatusCode);
            var res = await response.Content.ReadFromJsonAsync<OAuth2TokenResponse>();
            if (res is null)
                return StatusCode(StatusCodes.Status500InternalServerError);

            var idTokenPayloadJson = Base64UrlEncoder.Decode(res.id_token.Split(".")[1]);
            var idTokenPayload = JsonSerializer.Deserialize<IdToken>(idTokenPayloadJson);
            if (idTokenPayload is null)
                return StatusCode(StatusCodes.Status500InternalServerError);

            var userId = idTokenPayload.sub;
            var user = await _db.Users.FirstOrDefaultAsync(x => x.TwitchId == userId);
            if (user is null)
            {
                user = new()
                {
                    TwitchId = userId,
                    AccessToken = res.access_token,
                    RefreshToken = res.refresh_token,
                    IdToken = res.id_token
                };
                _db.Users.Add(user);
                await _db.SaveChangesAsync();
            }
            else
            {
                user.AccessToken = res.access_token;
                user.RefreshToken = res.refresh_token;
                user.IdToken = res.id_token;
                await _db.SaveChangesAsync();
            }

            var jwt = _jwtService.CreateTokenForUser(user);

            _logger.LogInformation("Successful login from user with twitch id " + user.TwitchId + "\njwt: " + jwt);

            return Json(new { token = jwt });
        }
        else
            return StatusCode(StatusCodes.Status403Forbidden);
    }
}
