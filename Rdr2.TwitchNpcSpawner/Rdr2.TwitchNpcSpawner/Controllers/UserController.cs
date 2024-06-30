namespace Rdr2.TwitchNpcSpawner.Controllers;

using Rdr2.TwitchNpcSpawner.ApiClient;
using Rdr2.TwitchNpcSpawner.Services;
using Microsoft.AspNetCore.Mvc;

[ApiController, Route("[controller]")]
public class UserController(JwtService _jwtService, TwitchApiClient _twitchApi) : Controller
{
    [HttpGet]
    public async Task<IActionResult> Get()
    {
        var user = await _jwtService.GetUser(Request.Headers.Authorization.ToString());
        if (user is null)
            return StatusCode(StatusCodes.Status401Unauthorized);

        var me = await _twitchApi.GetMe(user);
        if (me is null)
            return StatusCode(StatusCodes.Status500InternalServerError);
        if (!me.IsSuccessStatusCode)
            return StatusCode(me.StatusCode);

        var twitchUser = me.data.FirstOrDefault(x => x.id == user.TwitchId);
        if (twitchUser is null)
            return StatusCode(StatusCodes.Status404NotFound);

        return Json(twitchUser);
    }
}
